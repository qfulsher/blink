#include "display.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "tm1637.h"

#define TM1637_CLK_GPIO GPIO_NUM_5
#define TM1637_DIO_GPIO GPIO_NUM_4

static tm1637_handle_t s_display;
static SemaphoreHandle_t s_lock;
static int64_t s_override_expires_us = 0;

static bool override_active(void)
{
    return esp_timer_get_time() < s_override_expires_us;
}

void display_init(void)
{
    tm1637_config_t cfg = {
        .clk_pin = TM1637_CLK_GPIO,
        .dio_pin = TM1637_DIO_GPIO,
        .bit_delay_us = 100,
    };
    ESP_ERROR_CHECK(tm1637_init(&cfg, &s_display));
    ESP_ERROR_CHECK(tm1637_set_brightness(s_display, 5, true));
    s_lock = xSemaphoreCreateMutex();
}

void display_show_value(uint8_t value)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    tm1637_show_number(s_display, value, false, 4, 0);
    xSemaphoreGive(s_lock);
}

void display_show_time(uint8_t hours, uint8_t minutes)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (override_active()) {
        xSemaphoreGive(s_lock);
        return;
    }

    hours %= 100;
    minutes %= 100;

    uint8_t segs[4] = {
        tm1637_encode_digit(hours / 10),
        tm1637_encode_digit(hours % 10) | TM1637_SEG_DP, // colon
        tm1637_encode_digit(minutes / 10),
        tm1637_encode_digit(minutes % 10),
    };
    tm1637_set_segments(s_display, segs, 4, 0);
    xSemaphoreGive(s_lock);
}

void display_show_value_temp(uint8_t value, uint32_t duration_ms)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_override_expires_us = esp_timer_get_time() + (int64_t)duration_ms * 1000;
    tm1637_show_number(s_display, value, false, 4, 0);
    xSemaphoreGive(s_lock);
}
