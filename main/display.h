#pragma once
#include <stdint.h>

/**
 * Initialize the 7-segment display. Call once at startup before any other
 * display_* function.
 */
void display_init(void);

/**
 * Render a value as a 4-digit number. Does NOT respect overrides — use only
 * for one-off / foreground updates.
 */
void display_show_value(uint8_t value);

/**
 * Render HH:MM with the colon lit. This is treated as a "background" render:
 * if an override is currently active (see display_show_value_temp), this call
 * is silently dropped and the override continues to be shown until it expires.
 */
void display_show_time(uint8_t hours, uint8_t minutes);

/**
 * Render `value` and pin it on the display for `duration_ms`. Background
 * renderers (display_show_time) will no-op while this override is active.
 * Calling again before expiry replaces the previous override and restarts
 * the timer.
 */
void display_show_value_temp(uint8_t value, uint32_t duration_ms);
