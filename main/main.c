#include <stdlib.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "display.h"
#include "led.h"
#include "wifi.h"
#include "web.h"
#include "clock.h"
#include "sd.h"
#include "nvs_flash.h"
#include "time.h"

static const char *TAG = "example";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);
    tzset();

    led_init();
    display_init();
    sd_init();
    wifi_init_sta();
    init_sntp();
    web_init();
    clock_start();

    ESP_LOGI(TAG, "init complete; subsystems running on their own tasks");
}
