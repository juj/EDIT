#ifndef _SCREEN_H
#define _SCREEN_H

#include "memory.h"

#ifdef __C64__
// C64 colors: https://www.c64-wiki.com/wiki/Color
#define BLACK      0
#define WHITE      1
#define RED        2
#define CYAN       3
#define MAGENTA    4
#define GREEN      5
#define BLUE       6
#define YELLOW     7
#define BROWN      9

#define LTRED     10
#define DKGRAY    12
#define LTGREEN   13
#define LTBLUE    14
#define LTGRAY    15

// Don't exist on DOS/PC
//#define BLACK_GRAY 11
//#define ORANGE      8

// Don't exist on C64:
//#define LTCYAN   
//#define LTMAGENTA


#else
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
//#define LTCYAN    11   // No C64 counterpart
#define LTRED     12
//#define LTMAGENTA 13   // No C64 counterpart
#define YELLOW    14
#define WHITE     15
#endif

#define BGCOLOR(x) ((x) << 4)

extern int currentVideoMode;
extern unsigned char SCREEN_WIDTH;
extern unsigned char SCREEN_HEIGHT;

#define CHAR_RIGHT_ARROW 0x1A
#define CHAR_MIDDLE_DOT ((char)0xFA)

//extern char far *screen;

void save_screen();
int saved_screen_matches_current_screen();

int init_screen();
void deinit_screen();

void put_screen(int linearPos, char ch);
void put_screen_color(int linearPos, unsigned color);
void put_screen(int linearPos, char ch, unsigned char color);
char get_screen(int linearPos);
unsigned char get_screen_color(int linearPos);

void clear_line(int screenPos, unsigned char ch);
void show_line(int screenPos, char huge *text, int textLen);
void show_line_with_colors(int screenPos, char huge *text, int textLen, unsigned char far *colors);

void show_line_with_sel(int screenPos, char huge *text, int textLen, int selStart, int selEnd, int drawStartX, int drawEndX);
void show_line_with_colors_with_sel(int screenPos, char huge *text, int textLen, unsigned char far *colors, int selStart, int selEnd, int drawStartX, int drawEndX);

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
