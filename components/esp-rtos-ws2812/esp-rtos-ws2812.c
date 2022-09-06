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


#include <esp-rtos-ws2812.h>


static inline IRAM uint32_t get_cycle_count() {
    uint32_t cycles;
    asm volatile("rsr %0,ccount" : "=a" (cycles));
    return cycles;
}

// http://naberius.de/2015/05/14/esp8266-gpio-output-performance/
static inline void esp_ws2812_WS2812B_SEND_1(int port)
{
    //800ns HIGH & 450ns LOW
    __asm__ volatile ("movi    a2, 0x60000304  \n"
                      "movi    a3, 0x60000308  \n"
                      "movi.n    a4, 4      \n"
                      "s32i    a4, a2, 0     \n"
                      "memw            \n"
                      "movi.n    a5, 14         \n"
                      "3:            \n"
                      "addi    a5, a5, -1    \n"
                      "bnez    a5, 3b      \n"
                      "nop.n           \n"
                      "nop.n           \n"
                      "nop.n           \n"
                      "nop.n           \n"
                      "nop.n           \n"
                      "s32i    a4, a3, 0     \n"
                      "memw            \n"
                      "movi.n    a5, 2         \n"
                      "4:            \n"
                      "addi    a5, a5, -1    \n"
                      "bnez    a5, 4b      \n"
                      :: "g" (port)
                      : "a2", "a3", "a4", "a5"
                      );
}

static inline void esp_ws2812_WS2812B_SEND_0(int port)
{
    //400ns HIGH & 850ns LOW
    __asm__ volatile ("movi    a2, 0x60000304  \n"
                      "movi    a3, 0x60000308  \n"
                      "movi    a4, 4      \n"
                      "s32i    a4, a2, 0     \n"
                      "memw            \n"
                      "movi    a5, 7         \n"
                      "1:            \n"
                      "addi    a5, a5, -1    \n"
                      "bnez    a5, 1b      \n"
                      "nop.n           \n"
                      "s32i    a4, a3, 0     \n"
                      "memw            \n"
                      "movi    a5, 10         \n"
                      "2:            \n"
                      "addi    a5, a5, -1    \n"
                      "bnez    a5, 2b      \n"
                      "nop.n           \n"
                      "nop.n           \n"
                      "nop.n           \n"
                      :: "g" (port)
                      : "a2", "a3", "a4", "a5"
                      );
}

static inline void esp_ws2812_send_byte(uint8_t gpio_num, uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (byte & 0x80) {
            esp_ws2812_WS2812B_SEND_1 (gpio_num);
        } else {
            esp_ws2812_WS2812B_SEND_0 (gpio_num);
        }
        byte <<= 1; // shift to next bit
    }
}


static inline
void esp_ws2812_send_pixel(uint8_t gpio_num, uint32_t rgb)
{
    //printf ("%s: %#x  %#x  %#x\n", __func__, (rgb & 0x00FF00) >> 8, (rgb & 0xFF0000) >> 16, (rgb & 0x0000FF) >> 0);
    
    esp_ws2812_send_byte(gpio_num, (rgb & 0x00FF00) >> 8);
    esp_ws2812_send_byte(gpio_num, (rgb & 0xFF0000) >> 16);
    esp_ws2812_send_byte(gpio_num, (rgb & 0x0000FF) >> 0);
}

void IRAM esp_ws2812_send_pixels (uint8_t gpio_num, uint32_t *rgbs, size_t count)
{
    
//    uint32_t start, finish;
    uint32_t rgb;

    taskENTER_CRITICAL();
    
//    start = get_cycle_count();
    for (size_t i = 0; i < count; i++) {
        rgb = rgbs[i];
        esp_ws2812_send_pixel(gpio_num, rgb);
    }
//    finish = get_cycle_count();
    
    taskEXIT_CRITICAL();
//    sdk_os_delay_us(50); // display the loaded colors
    
//    printf ("%s: cycles %d, micro second per pixel %f\n", __func__, finish-start, (finish-start)*12.5/1000/count);
}
