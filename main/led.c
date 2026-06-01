#include "led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "led_strip.h"

#include "display.h"

#define LED_GPIO                 6
#define LED_NUM_LEDS             16
#define LED_DISPLAY_OVERRIDE_MS  2000
#define LED_DEFAULT_WHITE        64   // default brightness for led_set(true)

static const char *TAG = "led";

static led_strip_handle_t s_strip;
static SemaphoreHandle_t s_lock;
static led_color_t s_color = {0, 0, 0, 0};

static bool color_is_on(led_color_t c)
{
    return c.r | c.g | c.b | c.w;
}

void led_init(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_NUM_LEDS,
        .led_model = LED_MODEL_SK6812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRBW,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip));
    ESP_ERROR_CHECK(led_strip_clear(s_strip));

    s_lock = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "ring initialised: %d LEDs on GPIO %d (SK6812 RGBW)", LED_NUM_LEDS, LED_GPIO);
}

void led_set_color(led_color_t color)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool was_on = color_is_on(s_color);
    bool is_on = color_is_on(color);
    s_color = color;

    if (is_on) {
        for (int i = 0; i < LED_NUM_LEDS; i++) {
            led_strip_set_pixel_rgbw(s_strip, i, color.r, color.g, color.b, color.w);
        }
        led_strip_refresh(s_strip);
    } else {
        led_strip_clear(s_strip);
    }
    xSemaphoreGive(s_lock);

    if (was_on != is_on) {
        display_show_value_temp(is_on ? 1 : 0, LED_DISPLAY_OVERRIDE_MS);
    }
}

led_color_t led_get_color(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    led_color_t c = s_color;
    xSemaphoreGive(s_lock);
    return c;
}

void led_set(bool on)
{
    led_color_t target = on
        ? (led_color_t){0, 0, 0, LED_DEFAULT_WHITE}
        : (led_color_t){0, 0, 0, 0};
    led_set_color(target);
}

bool led_get(void)
{
    return color_is_on(led_get_color());
}
