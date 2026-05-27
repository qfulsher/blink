idf_component_register(SRCS "web.c" "main.c" "led.c" "display.c" "wifi.c" "clock.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES "web_assets/index.html"
                                   "web_assets/style.css"
                                   "web_assets/app.js"
                       PRIV_REQUIRES esp_http_server
                                     esp_wifi
                                     esp_netif
                                     esp_event
                                     esp_timer
                                     nvs_flash
                                     lwip
                                     driver)
