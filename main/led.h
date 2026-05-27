#pragma once
#include <stdbool.h>
#include <stdint.h>

/**
 * RGBW color value. Each channel is 0-255.
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
} led_color_t;

/**
 * Initialize the SK6812 RGBW NeoPixel ring. Call once at startup.
 */
void led_init(void);

/**
 * Convenience on/off. ON lights every LED at a default warm white,
 * OFF clears them. Triggers a 7-segment override on actual transitions.
 */
void led_set(bool on);

/**
 * Returns whether any LED is currently lit (any channel non-zero).
 */
bool led_get(void);

/**
 * Set every LED in the ring to `color` and push to the strip. A transition
 * between "all off" and "any channel non-zero" triggers a 7-segment override.
 */
void led_set_color(led_color_t color);

/**
 * Get the last color the ring was set to. Returns {0,0,0,0} after a clear
 * or before any set call.
 */
led_color_t led_get_color(void);
