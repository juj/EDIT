#ifndef _CURSOR_H
#define _CURSOR_H

#include <stdint.h>

void toggle_insert(void);
bool insert_mode(void);
void set_cursor_pos(uint8_t x, uint8_t y);
void set_insert_mode(bool new_insert_mode);

#define CURSOR_NONE   0
#define CURSOR_NORMAL 1
#define CURSOR_INSERT 2
void set_cursor_style(uint8_t style);

#endif