//
//  main.h
//  esp-homekit-rgbic-strip
//
//  Created by David B Brown on 02/01/2022.
//  Copyright © 2022 David B Brown. All rights reserved.
//

#ifndef eps_rtos_ws2812_h
#define eps_rtos_ws2812_h

#include <stdio.h>
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

void esp_ws2812_send_pixels (uint8_t gpio_num, uint32_t *rgbs, size_t count);

#endif /* eps_rtos_ws2812_h */
