set(WEB_ASSETS index.html style.css app.js)
set(GZ_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/web_assets_gz)
file(MAKE_DIRECTORY ${GZ_OUTPUT_DIR})

set(EMBED_GZ_FILES "")
foreach(asset ${WEB_ASSETS})
    set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/web_assets/${asset})
    set(DST ${GZ_OUTPUT_DIR}/${asset}.gz)
    add_custom_command(
        OUTPUT ${DST}
        COMMAND ${PYTHON_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/gzip_file.py
                ${SRC} ${DST}
        DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gzip_file.py
        COMMENT "gzip ${asset} -> ${asset}.gz"
        VERBATIM
    )
    list(APPEND EMBED_GZ_FILES ${DST})
endforeach()

idf_component_register(SRCS "web.c" "main.c" "led.c" "display.c" "wifi.c" "clock.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES ${EMBED_GZ_FILES}
                       PRIV_REQUIRES esp_http_server
                                     esp_wifi
                                     esp_netif
                                     esp_event
                                     esp_timer
                                     nvs_flash
                                     lwip
                                     driver)
