#pragma once

//#define SUPPORT_C64_DATASSETTE 1
//#define SUPPORT_INSERT_MODE    1

void repaint_screen_lines(cursor_pos yStart, cursor_pos yEnd, cursor_pos xStart, cursor_pos xEnd); // TODO proper declaration location
void repaint_screen();
bool insert_new_line(bool cursorMovesToNewline);
bool scroll_viewport_horizontally_to_display_cursor(bool repaint);
Line far *find_last_viewport_start_line(cursor_pos *linesInView);
void scroll_to_beginning();
void scroll_to_end();
bool restrict_cursor_on_line(bool repaint);
void move_viewport_vertically(cursor_pos lines, bool cursor_moves_along);
