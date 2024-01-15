//
//  main.c
//  esp-homekit-rgbic-strip
//
//  Created by David B Brown on 02/01/2022.
//  Copyright © 2022 David B Brown. All rights reserved.
//

#include "main.h"
/*
 * Copyright 2018 David B Brown (@maccoylton)
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * Example of using esp-homekit library
 * to monitor power consumption using a 100A SCT-013-000
 *
 */

#define DEVICE_MANUFACTURER "maccoylton"
#define DEVICE_NAME "RGBIC-LED-STRIP"
#define DEVICE_MODEL "1"
#define DEVICE_SERIAL "123456780"
#define FW_VERSION "1.0"

#include <stdio.h>
#include <stdlib.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>


#include <adv_button.h>
#include <led_codes.h>
#include <udplogger.h>
#include <custom_characteristics.h>
#include <colour_conversion.h>
#include <shared_functions.h>

#include <WS2812FX.h>
#include <esp-rtos-ws2812.h>

// add this section to make your device OTA capable
// create the extra characteristic &ota_trigger, at the end of the primary service (before the NULL)
// it can be used in Eve, which will show it, where Home does not
// and apply the four other parameters in the accessories_information section

#include <ota-api.h>

#define SAVE_DELAY 2000
#define LED_COUNT 180                    // this is the number of WS2812B leds on the strip
#define LED_SCALE_FACTOR 2.54            // this is the number of WS2812B leds on the strip
#define LED_STRIP_GPIO 2

int led_off_value=-1; /* global varibale to support LEDs set to 0 where the LED is connected to GND, 1 where +3.3v */

const int status_led_gpio = 2; /*set the gloabl variable for the led to be used for showing status */

// Global variables
float led_hue = 0;              // hue is scaled 0 to 360
float led_saturation = 59;      // saturation is scaled 0 to 100
float led_brightness = 100;     // brightness is scaled 0 to 100
bool led_on = false;            // on is boolean on or off
uint8_t rgbic_mode=0;          // used for the effect on the lights
uint8_t rgbic_mode_speed=1;



void rgbic_effect_set (homekit_value_t value);
void rgbic_effect_speed_set (homekit_value_t value);
void rgbic_led_saturation_set(homekit_value_t value) ;
homekit_value_t rgbic_led_saturation_get();
void rgbic_led_hue_set(homekit_value_t value) ;
homekit_value_t rgbic_led_hue_get () ;
homekit_value_t rgbic_led_brightness_get();
void rgbic_led_brightness_set(homekit_value_t value);
homekit_value_t rgbic_led_on_get ();
void rgbic_led_on_set (homekit_value_t value);
void colours_test_set (homekit_value_t value);
void rgbic_led_count_set (homekit_value_t value);


TaskHandle_t timing_test_task_handle = NULL;


homekit_characteristic_t wifi_check_interval   = HOMEKIT_CHARACTERISTIC_(CUSTOM_WIFI_CHECK_INTERVAL, 10, .setter=wifi_check_interval_set);
/* checks the wifi is connected and flashes status led to indicated connected */
homekit_characteristic_t task_stats   = HOMEKIT_CHARACTERISTIC_(CUSTOM_TASK_STATS, false , .setter=task_stats_set);
homekit_characteristic_t wifi_reset   = HOMEKIT_CHARACTERISTIC_(CUSTOM_WIFI_RESET, false, .setter=wifi_reset_set);
homekit_characteristic_t ota_beta     = HOMEKIT_CHARACTERISTIC_(CUSTOM_OTA_BETA, false, .setter=ota_beta_set);
homekit_characteristic_t lcm_beta    = HOMEKIT_CHARACTERISTIC_(CUSTOM_LCM_BETA, false, .setter=lcm_beta_set);
homekit_characteristic_t preserve_state   = HOMEKIT_CHARACTERISTIC_(CUSTOM_PRESERVE_STATE, false, .setter=preserve_state_set);

homekit_characteristic_t ota_trigger  = API_OTA_TRIGGER;
homekit_characteristic_t name         = HOMEKIT_CHARACTERISTIC_(NAME, DEVICE_NAME);
homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER,  DEVICE_MANUFACTURER);
homekit_characteristic_t serial       = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, DEVICE_SERIAL);
homekit_characteristic_t model        = HOMEKIT_CHARACTERISTIC_(MODEL,         DEVICE_MODEL);
homekit_characteristic_t revision     = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION,  FW_VERSION);

homekit_characteristic_t on = HOMEKIT_CHARACTERISTIC_(ON, true,
                                                      .getter = rgbic_led_on_get,
                                                      .setter = rgbic_led_on_set);

homekit_characteristic_t brightness = HOMEKIT_CHARACTERISTIC_(BRIGHTNESS, 100,
                                                              .getter = rgbic_led_brightness_get,
                                                              .setter = rgbic_led_brightness_set);

homekit_characteristic_t hue = HOMEKIT_CHARACTERISTIC_(HUE, 0,
                                                       .getter = rgbic_led_hue_get,
                                                       .setter = rgbic_led_hue_set);

homekit_characteristic_t saturation = HOMEKIT_CHARACTERISTIC_(SATURATION, 0,
                                                              .getter = rgbic_led_saturation_get,
                                                              .setter = rgbic_led_saturation_set);

homekit_characteristic_t rgbic_effect = HOMEKIT_CHARACTERISTIC_(CUSTOM_RGBIC_EFFECT, 0 , .setter=rgbic_effect_set);
homekit_characteristic_t rgbic_effect_speed = HOMEKIT_CHARACTERISTIC_(CUSTOM_RGBIC_EFFECT_SPEED, 1 , .setter=rgbic_effect_speed_set);
homekit_characteristic_t rgbic_led_count = HOMEKIT_CHARACTERISTIC_(CUSTOM_RGBIC_LED_COUNT, 480 , .setter=rgbic_led_count_set);
homekit_characteristic_t colours_test   = HOMEKIT_CHARACTERISTIC_(CUSTOM_COLOURS_GPIO_TEST, false , .setter=colours_test_set);

ETSTimer resize_strip_timer;




void timing_test_task(){
    
    uint32_t rainbow[]={0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF0000, 0x00FF00, 0x0000FF};
    uint32_t blackout[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    
    while (true){
        esp_ws2812_send_pixels(4,rainbow,LED_COUNT);
        vTaskDelay((3000) / portTICK_PERIOD_MS);
        esp_ws2812_send_pixels(4,blackout,LED_COUNT);
        vTaskDelay((3000) / portTICK_PERIOD_MS);
    }
}



void gpio_init() {
    gpio_enable(LED_STRIP_GPIO, GPIO_OUTPUT);
}


void rgbic_led_on_set (homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid on-value format: %d\n", value.format);
        return;
    }
    
    led_on = value.bool_value;
    on.value.bool_value =value.bool_value;
    if (led_on) {
        WS2812FX_start();
    } else {
        WS2812_clear();
        WS2812FX_stop();
    }
    sdk_os_timer_arm (&save_timer, SAVE_DELAY, 0 );
}


homekit_value_t rgbic_led_on_get () {
    return HOMEKIT_BOOL(led_on);
}


void rgbic_led_brightness_set(homekit_value_t value) {
    if (value.format != homekit_format_int) {
        printf("Invalid brightness-value format: %d\n", value.format);
        return;
    }
    brightness.value.int_value = value.int_value;
    led_brightness = value.int_value;
    
    printf ("B=%f\n", led_brightness);
    WS2812FX_setBrightness((uint8_t)floor(led_brightness*LED_SCALE_FACTOR));
    sdk_os_timer_arm (&save_timer, SAVE_DELAY, 0 );

}


homekit_value_t rgbic_led_brightness_get(){
    return HOMEKIT_INT(led_brightness);
}


homekit_value_t rgbic_led_hue_get () {
    return HOMEKIT_FLOAT(led_hue);
}


void rgbic_led_hue_set(homekit_value_t value) {
    if (value.format != homekit_format_float) {
        printf("Invalid hue-value format: %d\n", value.format);
        return;
    }
    hue.value.int_value = value.int_value;
    led_hue = value.float_value;
    
    rgb_color_t rgb = { { 0, 0, 0, 0 } };
    HSVtoRGB(led_hue, led_saturation, led_brightness, &rgb);
    
    printf ("H=%f, S=%f, B=%f, R=%d, G=%d, B=%d\n", led_hue, led_saturation, led_brightness, rgb.red, rgb.green, rgb.blue);
    
    WS2812FX_setColor(rgb.red, rgb.green, rgb.blue);
    sdk_os_timer_arm (&save_timer, SAVE_DELAY, 0 );

}


homekit_value_t rgbic_led_saturation_get(){
    return HOMEKIT_FLOAT(led_saturation);
}


void rgbic_led_saturation_set(homekit_value_t value) {
    if (value.format != homekit_format_float) {
        printf("Invalid sat-value format: %d\n", value.format);
        return;
    }
    saturation.value.int_value = value.int_value;
    led_saturation = value.float_value;
    
    rgb_color_t rgb = { { 0, 0, 0, 0 } };
    hsi2rgb(led_hue, led_saturation, led_brightness, &rgb);
    
    printf ("H=%f, S=%f, B=%f, R=%d, G=%d, B=%d\n", led_hue, led_saturation, led_brightness, rgb.red, rgb.green, rgb.blue);
    
    WS2812FX_setColor(rgb.red, rgb.green, rgb.blue);
    sdk_os_timer_arm (&save_timer, SAVE_DELAY, 0 );

}

void rgbic_effect_set (homekit_value_t value) {
    if (value.format != homekit_format_uint8) {
        printf("%s: Invalid effect-value format: %d\n", __func__, value.format);
        return;
    }
    rgbic_effect.value.int_value = value.int_value;
    rgbic_mode = value.int_value;
    WS2812FX_setMode(rgbic_mode);
    sdk_os_timer_arm (&save_timer, SAVE_DELAY, 0 );

}

void rgbic_effect_speed_set (homekit_value_t value) {
    if (value.format != homekit_format_uint8) {
        printf("%s: Invalid effect-value format: %d\n", __func__, value.format);
        return;
    }
    rgbic_mode_speed = value.int_value;
    rgbic_effect_speed.value.int_value = value.int_value;
    WS2812FX_setSpeed(rgbic_mode_speed);
    sdk_os_timer_arm (&save_timer, SAVE_DELAY, 0 );

}


void resize_strip ( ){
    
    printf("%s: To: %d\n", __func__, rgbic_led_count.value.int_value);
    //WS2812FX_resize (rgbic_led_count.value.int_value);
    save_characteristics();
    sdk_system_restart();
    printf("%s: End\n", __func__);


}


void rgbic_led_count_set (homekit_value_t value) {
    if (value.format != homekit_format_uint16) {
        printf("%s: Invalid led count value: %d\n", __func__, value.format);
        return;
    }
    rgbic_led_count.value.int_value = value.int_value;

    sdk_os_timer_arm (&resize_strip_timer, SAVE_DELAY-50, 0 );
    sdk_os_timer_arm (&save_timer, SAVE_DELAY, 0 );
    
}


void colours_test_set (homekit_value_t value)
{
    
    if ( value.bool_value) {
        vTaskSuspend(fx_service_task_handle);
        xTaskCreate(timing_test_task, "timing_test_task", 512 , NULL, tskIDLE_PRIORITY+2, &timing_test_task_handle);
    }  else {
        if (timing_test_task_handle != NULL) {
            vTaskDelete (timing_test_task_handle);
        }
        vTaskResume (fx_service_task_handle);
    }
    
}

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            &name,
            &manufacturer,
            &serial,
            &model,
            &revision,
            HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
            NULL
        }),
        
        
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Switch"),
            &on,
            &saturation,
            &hue,
            &brightness,
            &rgbic_effect,
            &rgbic_effect_speed,
            &rgbic_led_count,
            &ota_trigger,
            &wifi_reset,
            &ota_beta,
            &lcm_beta,
            &task_stats,
            &wifi_check_interval,
            &preserve_state,
            &colours_test,
            NULL
        }),
        
        
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111",
    .setupId = "1234",
    .on_event = on_homekit_event
};


/*void eve_history_send_log(uint32_t starting_from_address){
    
    
}*/


void recover_from_reset (int reason){
    /* called if we restarted abnormally */
    printf ("%s: reason %d\n", __func__, reason);
}

void save_characteristics ( ){
    
    printf ("%s:\n", __func__);
    save_characteristic_to_flash(&preserve_state, preserve_state.value);
    if ( preserve_state.value.bool_value == true){
        printf ("%s:Preserving state\n", __func__);
        save_characteristic_to_flash(&on, on.value);
        save_characteristic_to_flash(&saturation, saturation.value);
        save_characteristic_to_flash(&hue, hue.value);
        save_characteristic_to_flash(&brightness, brightness.value);
        save_characteristic_to_flash(&rgbic_effect, rgbic_effect.value);
        save_characteristic_to_flash(&rgbic_effect_speed, rgbic_effect_speed.value);
        save_characteristic_to_flash(&wifi_check_interval, wifi_check_interval.value);
        save_characteristic_to_flash(&rgbic_led_count, rgbic_led_count.value);
    } else {
        printf ("%s:Not preserving state\n", __func__);
    }
}


void accessory_init_not_paired (void) {
    /* initalise anything you don't want started until wifi and homekit imitialisation is confirmed, but not paired */
    
}
    

void accessory_init (void ){
    /* initalise anything you don't want started until wifi and pairing is confirmed */
    
    if ( preserve_state.value.bool_value == true){
        printf ("%s:Loading preserved state\n", __func__);
        load_characteristic_from_flash(&on);
        load_characteristic_from_flash(&saturation);
        load_characteristic_from_flash(&hue);
        load_characteristic_from_flash(&brightness);
        load_characteristic_from_flash(&rgbic_effect);
        load_characteristic_from_flash(&rgbic_effect_speed);
        load_characteristic_from_flash(&rgbic_led_count);
    } else {
        printf ("%s:Preserved state is off\n", __func__);
    }
    homekit_characteristic_notify(&preserve_state, preserve_state.value);
    homekit_characteristic_notify(&wifi_check_interval, wifi_check_interval.value);
    homekit_characteristic_notify(&on, on.value);
    homekit_characteristic_notify(&saturation, saturation.value);
    homekit_characteristic_notify(&hue, hue.value);
    homekit_characteristic_notify(&brightness, brightness.value);
    homekit_characteristic_notify(&rgbic_effect, rgbic_effect.value);
    homekit_characteristic_notify(&rgbic_effect_speed, rgbic_effect_speed.value);
    
    rgbic_led_saturation_set (saturation.value);
    rgbic_led_hue_set (hue.value);
    rgbic_led_brightness_set (brightness.value);
    rgbic_effect_set (rgbic_effect.value);
    rgbic_effect_speed_set (rgbic_effect_speed.value);
    rgbic_led_on_set (on.value);
    
    sdk_os_timer_setfn(&resize_strip_timer, resize_strip, NULL);

    
    WS2812FX_init_non_i2s( rgbic_led_count.value.int_value, LED_STRIP_GPIO);

    

}


void user_init(void) {
    
    standard_init (&name, &manufacturer, &model, &serial, &revision);
    
    gpio_init();
    
    wifi_config_init(DEVICE_NAME, NULL, on_wifi_ready);
    
}
