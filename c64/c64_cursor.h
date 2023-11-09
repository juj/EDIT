#pragma once

#include <stdint.h>

#define CURSOR_NONE   0
#define CURSOR_NORMAL 1
#define CURSOR_INSERT 2
void c64_set_cursor_style(uint8_t style);
void c64_set_cursor_pos(uint8_t x, uint8_t y);
uint16_t c64_get_cursor_linear_pos();
