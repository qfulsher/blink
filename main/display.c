#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tm1637.h"
#define TM1637_CLK_GPIO GPIO_NUM_5
#define TM1637_DIO_GPIO GPIO_NUM_4
static tm1637_handle_t s_display;

static void display_init(void)
{
    tm1637_config_t cfg = {
        .clk_pin = TM1637_CLK_GPIO,
        .dio_pin = TM1637_DIO_GPIO,
        .bit_delay_us = 100,
    };
    ESP_ERROR_CHECK(tm1637_init(&cfg, &s_display));
    ESP_ERROR_CHECK(tm1637_set_brightness(s_display, 5, true));
}

static void display_show_value(uint8_t value) {
    tm1637_show_number(s_display, value, false, 4, 0);
}