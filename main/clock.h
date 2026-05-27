#pragma once

/**
 * Spawn a background task that updates the 7-segment display with the current
 * local time once per second. Call once after display_init() and SNTP setup.
 */
void clock_start(void);
