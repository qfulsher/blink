#include "sd.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"

#define SD_SPI_HOST    SPI2_HOST
#define SD_PIN_MISO    21
#define SD_PIN_MOSI    20
#define SD_PIN_SCK     19
#define SD_PIN_CS      18
#define SD_MAX_PATH    256

static const char *TAG = "sd";
static sdmmc_card_t *s_card = NULL;

void sd_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_MOSI,
        .miso_io_num = SD_PIN_MISO,
        .sclk_io_num = SD_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    esp_err_t ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_CS;
    slot_config.host_id = SD_SPI_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
    };

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_cfg, &s_card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGW(TAG, "no SD card detected or filesystem unreadable; continuing without SD");
        } else {
            ESP_LOGE(TAG, "sd mount failed: %s", esp_err_to_name(ret));
        }
        return;
    }

    sdmmc_card_print_info(stdout, s_card);
    ESP_LOGI(TAG, "SD mounted at %s", SD_MOUNT_POINT);
}

static void build_abs(char *out, size_t n, const char *rel)
{
    snprintf(out, n, "%s/%s", SD_MOUNT_POINT, rel);
}

bool sd_exists(const char *path)
{
    char abs[SD_MAX_PATH];
    build_abs(abs, sizeof(abs), path);
    struct stat st;
    return stat(abs, &st) == 0;
}

int sd_delete(const char *path)
{
    char abs[SD_MAX_PATH];
    build_abs(abs, sizeof(abs), path);
    return unlink(abs);
}

int sd_write_bytes(const char *path, const void *buf, size_t len)
{
    char abs[SD_MAX_PATH];
    build_abs(abs, sizeof(abs), path);
    FILE *f = fopen(abs, "wb");
    if (!f) {
        ESP_LOGE(TAG, "open for write failed: %s (errno %d: %s)",
                 abs, errno, strerror(errno));
        return -1;
    }
    size_t written = fwrite(buf, 1, len, f);
    fclose(f);
    if (written != len) {
        ESP_LOGE(TAG, "short write to %s (%zu/%zu)", abs, written, len);
        return -1;
    }
    return (int)written;
}

int sd_read_bytes(const char *path, void *buf, size_t maxlen)
{
    char abs[SD_MAX_PATH];
    build_abs(abs, sizeof(abs), path);
    FILE *f = fopen(abs, "rb");
    if (!f) {
        ESP_LOGE(TAG, "open for read failed: %s (errno %d: %s)",
                 abs, errno, strerror(errno));
        return -1;
    }
    size_t n = fread(buf, 1, maxlen, f);
    fclose(f);
    return (int)n;
}
