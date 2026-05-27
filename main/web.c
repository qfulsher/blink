#include "web.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "mdns.h"
#include "cJSON.h"

#include "led.h"

static const char *TAG = "web";

extern const uint8_t index_html_gz_start[] asm("_binary_index_html_gz_start");
extern const uint8_t index_html_gz_end[]   asm("_binary_index_html_gz_end");
extern const uint8_t style_css_gz_start[]  asm("_binary_style_css_gz_start");
extern const uint8_t style_css_gz_end[]    asm("_binary_style_css_gz_end");
extern const uint8_t app_js_gz_start[]     asm("_binary_app_js_gz_start");
extern const uint8_t app_js_gz_end[]       asm("_binary_app_js_gz_end");

// Sends a pre-gzipped asset with the correct Content-Type and the
// Content-Encoding: gzip header so the browser decompresses transparently.
static esp_err_t send_static_gz(httpd_req_t *req, const uint8_t *start,
                                const uint8_t *end, const char *content_type)
{
    httpd_resp_set_type(req, content_type);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)start, end - start);
}

static esp_err_t index_handler(httpd_req_t *req)
{
    return send_static_gz(req, index_html_gz_start, index_html_gz_end, "text/html");
}

static esp_err_t css_handler(httpd_req_t *req)
{
    return send_static_gz(req, style_css_gz_start, style_css_gz_end, "text/css");
}

static esp_err_t js_handler(httpd_req_t *req)
{
    return send_static_gz(req, app_js_gz_start, app_js_gz_end, "application/javascript");
}

static esp_err_t send_led_state(httpd_req_t *req, bool on)
{
    char body[32];
    int n = snprintf(body, sizeof(body), "{\"on\":%s}", on ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, body, n);
}

static esp_err_t led_get_handler(httpd_req_t *req)
{
    return send_led_state(req, led_get());
}

static esp_err_t led_post_handler(httpd_req_t *req)
{
    if (req->content_len == 0 || req->content_len > 64) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "body must be 1-64 bytes");
        return ESP_FAIL;
    }

    char buf[65];
    int received = httpd_req_recv(req, buf, req->content_len);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "failed to read body");
        return ESP_FAIL;
    }
    buf[received] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
        return ESP_FAIL;
    }
    cJSON *on_item = cJSON_GetObjectItem(root, "on");
    if (!cJSON_IsBool(on_item)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "expected {\"on\": true|false}");
        return ESP_FAIL;
    }
    bool desired = cJSON_IsTrue(on_item);
    cJSON_Delete(root);

    led_set(desired);
    ESP_LOGI(TAG, "LED set to %s via API", desired ? "ON" : "OFF");
    return send_led_state(req, desired);
}

static esp_err_t send_led_color(httpd_req_t *req, led_color_t c)
{
    char body[64];
    int n = snprintf(body, sizeof(body),
                     "{\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u}",
                     c.r, c.g, c.b, c.w);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, body, n);
}

static esp_err_t led_color_get_handler(httpd_req_t *req)
{
    return send_led_color(req, led_get_color());
}

// Reads an optional 0-255 channel from `root`. Returns true on success;
// false sends a 400 and returns ESP_FAIL via `*err`.
static bool read_channel(cJSON *root, const char *key, uint8_t *out,
                         httpd_req_t *req, esp_err_t *err)
{
    cJSON *item = cJSON_GetObjectItem(root, key);
    if (!item) {
        *out = 0;
        return true;
    }
    if (!cJSON_IsNumber(item)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "channels must be numbers");
        *err = ESP_FAIL;
        return false;
    }
    int v = item->valueint;
    if (v < 0 || v > 255) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "channels must be 0-255");
        *err = ESP_FAIL;
        return false;
    }
    *out = (uint8_t)v;
    return true;
}

static esp_err_t led_color_post_handler(httpd_req_t *req)
{
    if (req->content_len == 0 || req->content_len > 128) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "body must be 1-128 bytes");
        return ESP_FAIL;
    }

    char buf[129];
    int received = httpd_req_recv(req, buf, req->content_len);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "failed to read body");
        return ESP_FAIL;
    }
    buf[received] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
        return ESP_FAIL;
    }

    led_color_t color;
    esp_err_t err = ESP_OK;
    if (!read_channel(root, "r", &color.r, req, &err)) return err;
    if (!read_channel(root, "g", &color.g, req, &err)) return err;
    if (!read_channel(root, "b", &color.b, req, &err)) return err;
    if (!read_channel(root, "w", &color.w, req, &err)) return err;
    cJSON_Delete(root);

    led_set_color(color);
    ESP_LOGI(TAG, "ring color set to r=%u g=%u b=%u w=%u via API",
             color.r, color.g, color.b, color.w);
    return send_led_color(req, color);
}

static void start_mdns(void)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("openHatch"));
    ESP_ERROR_CHECK(mdns_instance_name_set("openHatch ESP32"));
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
}

void web_init(void)
{
    start_mdns();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    const httpd_uri_t routes[] = {
        { .uri = "/",              .method = HTTP_GET,  .handler = index_handler },
        { .uri = "/style.css",     .method = HTTP_GET,  .handler = css_handler },
        { .uri = "/app.js",        .method = HTTP_GET,  .handler = js_handler },
        { .uri = "/api/led",       .method = HTTP_GET,  .handler = led_get_handler },
        { .uri = "/api/led",       .method = HTTP_POST, .handler = led_post_handler },
        { .uri = "/api/led/color", .method = HTTP_GET,  .handler = led_color_get_handler },
        { .uri = "/api/led/color", .method = HTTP_POST, .handler = led_color_post_handler },
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &routes[i]));
    }

    ESP_LOGI(TAG, "HTTP server up at http://openHatch.local");
}
