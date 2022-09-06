# Component makefile for esp-rtos-ws2812


INC_DIRS += $(esp-rtos-ws2812_ROOT)


esp-rtos-ws2812 = $(esp-rtos-ws2812_ROOT)
esp-rtos-ws2812_SRC_DIR = $(esp-rtos-ws2812_ROOT)

$(eval $(call component_compile_rules,esp-rtos-ws2812))
