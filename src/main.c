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
#define DEVICE_NAME "Power Monitor"
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
#include <math.h>


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
#include <ws2812_i2s/ws2812_i2s.h>
// add this section to make your device OTA capable
// create the extra characteristic &ota_trigger, at the end of the primary service (before the NULL)
// it can be used in Eve, which will show it, where Home does not
// and apply the four other parameters in the accessories_information section

#include <ota-api.h>

#define SAVE_DELAY 2000
#define LED_COUNT 60            // this is the number of WS2812B leds on the strip
#define LED_SCALE_FACTOR 2.55            // this is the number of WS2812B leds on the strip


int led_off_value=-1; /* global varibale to support LEDs set to 0 where the LED is connected to GND, 1 where +3.3v */

const int status_led_gpio = 2; /*set the gloabl variable for the led to be used for showing status */

// Global variables
float led_hue = 0;              // hue is scaled 0 to 360
float led_saturation = 59;      // saturation is scaled 0 to 100
float led_brightness = 100;     // brightness is scaled 0 to 100
bool led_on = false;            // on is boolean on or off
uint8_t rgbic_mode=0;         // used fo the effect on the lights

void rgbic_led_on_set (homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid on-value format: %d\n", value.format);
        return;
    }
    
    led_on = value.bool_value;
    
    if (led_on) {
        WS2812FX_setBrightness((uint8_t)floor(led_brightness*LED_SCALE_FACTOR));
        
    } else {
        WS2812FX_setBrightness(0);
    }
}


homekit_value_t rgbic_led_on_get () {
    return HOMEKIT_BOOL(led_on);
}


void rgbic_led_brightness_set(homekit_value_t value) {
    if (value.format != homekit_format_int) {
        printf("Invalid brightness-value format: %d\n", value.format);
        return;
    }
    led_brightness = value.int_value;
    
    WS2812FX_setBrightness((uint8_t)floor(led_brightness*LED_SCALE_FACTOR));
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
    led_hue = value.float_value;
    
    rgb_color_t rgb = { { 0, 0, 0, 0 } };
    hsi2rgb(led_hue, led_saturation, 100, &rgb);
    
    WS2812FX_setColor(rgb.red, rgb.green, rgb.blue);
}


homekit_value_t rgbic_led_saturation_get(){
    return HOMEKIT_FLOAT(led_saturation);
}


void rgbic_led_saturation_set(homekit_value_t value) {
    if (value.format != homekit_format_float) {
        printf("Invalid sat-value format: %d\n", value.format);
        return;
    }
    led_saturation = value.float_value;
    
    rgb_color_t rgb = { { 0, 0, 0, 0 } };
    hsi2rgb(led_hue, led_saturation, 100, &rgb);
    
    WS2812FX_setColor(rgb.red, rgb.green, rgb.blue);
}

void rgbic_eeffect_set (homekit_value_t value) {
    if (value.format != homekit_format_int) {
        printf("Invalid effect-value format: %d\n", value.format);
        return;
    }
    rgbic_mode = value.int_value;
    WS2812FX_setMode360(rgbic_mode);
}


homekit_characteristic_t wifi_check_interval   = HOMEKIT_CHARACTERISTIC_(CUSTOM_WIFI_CHECK_INTERVAL, 10, .setter=wifi_check_interval_set);
/* checks the wifi is connected and flashes status led to indicated connected */
homekit_characteristic_t task_stats   = HOMEKIT_CHARACTERISTIC_(CUSTOM_TASK_STATS, false , .setter=task_stats_set);
homekit_characteristic_t wifi_reset   = HOMEKIT_CHARACTERISTIC_(CUSTOM_WIFI_RESET, false, .setter=wifi_reset_set);
homekit_characteristic_t ota_beta     = HOMEKIT_CHARACTERISTIC_(CUSTOM_OTA_BETA, false, .setter=ota_beta_set);
homekit_characteristic_t lcm_beta    = HOMEKIT_CHARACTERISTIC_(CUSTOM_LCM_BETA, false, .setter=lcm_beta_set);

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

homekit_characteristic_t rgbic_effect = HOMEKIT_CHARACTERISTIC_(CUSTOM_RGBIC_EFFECT, 1 , .setter=rgbic_eeffect_set);


void gpio_init() {
    
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_sensor, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            &name,
            &manufacturer,
            &serial,
            &model,
            &revision,
            HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
            NULL
        }),
        
        
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Switch"),

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


void eve_history_send_log(uint32_t starting_from_address){
    
    
}


void recover_from_reset (int reason){
    /* called if we restarted abnormally */
    printf ("%s: reason %d\n", __func__, reason);
}

void save_characteristics ( ){
    
    printf ("%s:\n", __func__);
    save_characteristic_to_flash(&wifi_check_interval, wifi_check_interval.value);
}


void accessory_init_not_paired (void) {
    /* initalise anything you don't want started until wifi and homekit imitialisation is confirmed, but not paired */
    
}


void accessory_init (void ){
    /* initalise anything you don't want started until wifi and pairing is confirmed */
    
    load_characteristic_from_flash(&wifi_check_interval);
    WS2812FX_init(LED_COUNT);
}


void user_init(void) {
    
    standard_init (&name, &manufacturer, &model, &serial, &revision);
    
    gpio_init();
    
    wifi_config_init(DEVICE_NAME, NULL, on_wifi_ready);
    
}
