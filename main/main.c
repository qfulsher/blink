/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "tm1637.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "regex.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "display.h"
#include "led.h"

static const char *TAG = "example";

static uint8_t s_led_state = 0;

void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    led_init();
    display_init();

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        
        led_set(s_led_state);
        display_show_value(s_led_state);

        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
