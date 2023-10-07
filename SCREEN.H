#ifndef _SCREEN_H
#define _SCREEN_H

#include "memory.h"

#define ATTR_BLINK 0x80
#define BLACK      0
#define BLUE       1
#define GREEN      2
#define CYAN       3
#define RED        4
#define MAGENTA    5
#define BROWN      6
#define LTGRAY     7
#define DKGRAY     8
#define LTBLUE     9
#define LTGREEN   10
#define LTCYAN    11
#define LTRED     12
#define LTMAGENTA 13
#define YELLOW    14
#define WHITE     15

#define BGCOLOR(x) ((x) << 4)

extern int currentVideoMode;
extern unsigned char SCREEN_WIDTH;
extern unsigned char SCREEN_HEIGHT;

#define CHAR_RIGHT_ARROW 0x1A
#define CHAR_MIDDLE_DOT ((char)0xFA)

//extern char far *screen;

void clear_line(char far *screenPtr, unsigned char ch);
void show_line(char far *screenPtr, char huge *text, int textLen);
void show_line_with_colors(char far *screenPtr, char huge *text, int textLen, unsigned char far *colors);

void show_line_with_sel(char far *screenPtr, char huge *text, int textLen, int selStart, int selEnd);
void show_line_with_colors_with_sel(char far *screenPtr, char huge *text, int textLen, unsigned char far *colors, int selStart, int selEnd);

void put_string_on_screen(const char *str, int x, int y);
void put_string_on_screen(const char *str, int x, int y, unsigned char color);

void get_string_on_screen(int x, int y, int maxLength, char *dst);

void set_chars_color(int x, int y, int numChars, unsigned char color);
void highlight_line(int yLine, unsigned char bgColor);

void log_error(const char *errorText);

void clear_screen(unsigned char ch, unsigned char color);

void log_x(const char *text);

#define TEXT_MODE_40x25 0
#define TEXT_MODE_40x28 1
#define TEXT_MODE_40x50 2
#define TEXT_MODE_80x25 3
#define TEXT_MODE_80x28 4
#define TEXT_MODE_80x50 5
#define TEXT_MODE_90x60 6
#define LAST_VIDEO_MODE 6

void set_vga_text_mode(int mode);

void set_vga_font_height(int height);
void set_vga_cursor_position(int x, int y);

#endif
