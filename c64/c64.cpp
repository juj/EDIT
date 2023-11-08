#include "../iassert.h"
#include "../screen.h"
#include "c64_kernal.h"
#include <stdio.h>
#include <stdint.h>

// Color RAM: https://www.c64-wiki.com/wiki/Color_RAM
static volatile unsigned char * const color_ram = (unsigned char*)0xD800;
static volatile unsigned char * const screen_ram = (unsigned char*)0x0400;
static volatile unsigned char * const cursor_blink = (unsigned char *)0xCC;
static volatile unsigned char * const cursor_blink_delay = (unsigned char *)0xCD;
static volatile unsigned char * const char_under_cursor = (unsigned char *)0xCE;
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF;
static volatile unsigned char * const cursor_color = (unsigned char*)0x0286;
static volatile unsigned char * const color_under_cursor = (unsigned char*)0x0287;

#define CURSOR_NONE   0
#define CURSOR_NORMAL 1
#define CURSOR_INSERT 2
void set_cursor_style(uint8_t style)
{
  *cursor_blink = (style == CURSOR_NONE) ? 1 : 0;
}

static uint16_t cursorLinearPos;

void set_cursor_pos(uint8_t x, uint8_t y)
{
  _DISABLE_INTR();
  if (*cursor_blink_phase) // If cursor is showing a reverse image right now, clear the reverse
  {
    screen_ram[cursorLinearPos] = *char_under_cursor;
    color_ram[cursorLinearPos] = *color_under_cursor;
  }
  cursorLinearPos = (uint16_t)y*SCREEN_WIDTH+x;
  *cursor_color = color_ram[cursorLinearPos]; // Set new cursor color based on the color on new square
  _PLOT_SET(x, y);

  // When moving the cursor, it is more visually pleasing to make it immediately appear to the
  // new position. This helps distinguish where it is located while moving.
  *cursor_blink_phase = 1; // Make reverse character visible
  *cursor_blink_delay = 20; // And restore time until next blink
  *char_under_cursor = screen_ram[cursorLinearPos];
  *color_under_cursor = color_ram[cursorLinearPos];
  screen_ram[cursorLinearPos] |= 0x80; // Inverse char at cursor to make it visible
  _ENABLE_INTR();
}

unsigned char petscii_to_screen_code(unsigned char petscii);

void put_screen(uint16_t linearPos, char ch, unsigned char color)
{
  uint8_t newChar = petscii_to_screen_code(ch) ^ (color&0x80);

  // Gotcha: if we are setting the character under the cursor, we must
  // be super careful, or otherwise the C64 built-in cursor logic will
  // smear the wrong character on screen.
  if (linearPos == cursorLinearPos)
  {
    _DISABLE_INTR();
    // Odd C64 timing issue: must reset blink counter, or otherwise
    // C64 might smear the old character on screen.
    *cursor_blink_delay = 20;
    *char_under_cursor = screen_ram[linearPos] = newChar;
    *color_under_cursor = color_ram[linearPos] = color;
    _ENABLE_INTR();
    return;
  }
  screen_ram[linearPos] = newChar;
  color_ram[linearPos] = color;
}

void put_screen(uint16_t linearPos, char ch)
{
  put_screen(linearPos, ch, 7);
}

char get_screen(uint16_t linearPos)
{
  // Gotcha: when reading the screen position where cursor
  // is currently at, must read the character under cursor instead.
  if (linearPos == cursorLinearPos) return *char_under_cursor;
  return screen_ram[linearPos]; // TODO: screen code to PETSCII
}

void put_screen_color(uint16_t linearPos, unsigned char color)
{
  put_screen(linearPos, get_screen(linearPos), 7);
}

unsigned char get_screen_color(uint16_t linearPos)
{
  // Gotcha: when reading the screen position where cursor
  // is currently at, must read the color under cursor instead.
  if (linearPos == cursorLinearPos) return *color_under_cursor;
  return color_ram[linearPos];
}

static volatile unsigned char * const mem_setup_register = (unsigned char *)0xD018;

bool init_screen()
{
  _SCREEN(&SCREEN_WIDTH, &SCREEN_HEIGHT);

  // Switch charset into text mode, where both lower case and upper case characters
  // are available. Default register value at boot is 0x15.
  *mem_setup_register = 0x17;

  return true;
}

void deinit_screen()
{
}

static volatile unsigned char * const screen_frame = (unsigned char *)0xD020;

void highlight_scroll_to_right(bool scrolled)
{
  *screen_frame = scrolled ? /*DARKBLUE*/6 : /*LIGHTBLUE*/14;
}
