#include "clock.h"

#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display.h"

static void clock_task(void *arg)
{
    (void)arg;
    while (1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm now_tm;
        localtime_r(&tv.tv_sec, &now_tm);

        uint8_t hour12 = now_tm.tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        display_show_time(hour12, now_tm.tm_min);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void clock_start(void)
{
    xTaskCreate(clock_task, "clock", 3072, NULL, 5, NULL);
}
