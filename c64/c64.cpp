#include "../iassert.h"
#include "../screen.h"
#include "c64_kernal.h"
#include "c64_cursor.h"
#include <stdio.h>
#include <stdint.h>

// Color RAM: https://www.c64-wiki.com/wiki/Color_RAM
static volatile unsigned char * const color_ram = (unsigned char*)0xD800;
static volatile unsigned char * const screen_ram = (unsigned char*)0x0400;
//static volatile unsigned char * const screen_ram = (unsigned char*)0xCC00; // If using custom font
static volatile unsigned char * const cursor_blink = (unsigned char *)0xCC;
static volatile unsigned char * const cursor_blink_delay = (unsigned char *)0xCD;
static volatile unsigned char * const char_under_cursor = (unsigned char *)0xCE;
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF;
static volatile unsigned char * const cursor_color = (unsigned char*)0x0286;
static volatile unsigned char * const color_under_cursor = (unsigned char*)0x0287;

unsigned char petscii_to_screen_code(unsigned char petscii);

void set_cursor_style(uint8_t style)
{
  c64_set_cursor_style(style);
}

void set_cursor_pos(uint8_t x, uint8_t y)
{
  c64_set_cursor_pos(x, y);
}

void put_screen(uint16_t linearPos, char ch, unsigned char color)
{
  uint8_t newChar = petscii_to_screen_code(ch) ^ (color&0x80);

  // Gotcha: if we are setting the character under the cursor, we must
  // be super careful, or otherwise the C64 built-in cursor logic will
  // smear the wrong character on screen.
  if (linearPos == c64_get_cursor_linear_pos())
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
  if (linearPos == c64_get_cursor_linear_pos()) return *char_under_cursor;
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
  if (linearPos == c64_get_cursor_linear_pos()) return *color_under_cursor;
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
