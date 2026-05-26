#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#define BLINK_GPIO CONFIG_BLINK_GPIO
#ifdef CONFIG_BLINK_LED_STRIP
static const char *TAG = "example";

static led_strip_handle_t led_strip;
static bool s_led_on = false;

void led_init(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#endif

void led_set(bool on)
{
    s_led_on = on;
    if (on) {
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        led_strip_refresh(led_strip);
    } else {
        led_strip_clear(led_strip);
    }
}

bool led_get(void)
{
    return s_led_on;
}
