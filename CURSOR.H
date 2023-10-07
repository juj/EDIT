#ifndef _CURSOR_H
#define _CURSOR_H

void toggle_insert(void);
void set_cursor_pos(int x, int y);

#define CURSOR_NONE   0
#define CURSOR_NORMAL 1
#define CURSOR_INSERT 2
void set_cursor_style(int style);

#endif