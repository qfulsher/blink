#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "display.h"
#include "led.h"
#include "wifi.h"
#include "web.h"
#include "nvs_flash.h"
#include <sys/time.h>
#include <time.h>

static const char *TAG = "example";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    led_init();
    display_init();
    wifi_init_sta();
    init_sntp();
    web_init();

    setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);
    tzset();

    while (1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm now_tm;
        localtime_r(&tv.tv_sec, &now_tm);
        uint8_t hour12 = now_tm.tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        display_show_time(hour12, now_tm.tm_min);

        ESP_LOGI(TAG, "%02d:%02d  LED=%s", hour12, now_tm.tm_min, led_get() ? "ON" : "OFF");
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
