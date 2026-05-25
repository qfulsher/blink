#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tm1637.h"
#define TM1637_CLK_GPIO GPIO_NUM_5
#define TM1637_DIO_GPIO GPIO_NUM_4
static tm1637_handle_t s_display;

void display_init(void)
{
    tm1637_config_t cfg = {
        .clk_pin = TM1637_CLK_GPIO,
        .dio_pin = TM1637_DIO_GPIO,
        .bit_delay_us = 100,
    };
    ESP_ERROR_CHECK(tm1637_init(&cfg, &s_display));
    ESP_ERROR_CHECK(tm1637_set_brightness(s_display, 5, true));
}

void display_show_value(uint8_t value) {
    tm1637_show_number(s_display, value, false, 4, 0);
}

void display_show_time(uint8_t hours, uint8_t minutes)
{
    hours %= 100;
    minutes %= 100;

    uint8_t segs[4] = {
        tm1637_encode_digit(hours / 10),
        tm1637_encode_digit(hours % 10) | TM1637_SEG_DP, // colon
        tm1637_encode_digit(minutes / 10),
        tm1637_encode_digit(minutes % 10),
    };
    tm1637_set_segments(s_display, segs, 4, 0);
}