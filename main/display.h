#pragma once
#include <stdint.h>

void display_init(void);
void display_show_value(uint8_t value);
void display_show_time(uint8_t hours, uint8_t minutes);