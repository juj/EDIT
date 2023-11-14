#include "../iassert.h"
#include "../screen.h"
#include "c64_kernal.h"
#include "c64_cursor.h"
#include <stdio.h>
#include <stdint.h>

// Color RAM: https://www.c64-wiki.com/wiki/Color_RAM
static volatile unsigned char * const color_ram = (unsigned char*)0xD800;
//static volatile unsigned char * const screen_ram = (unsigned char*)0x0400;
static volatile unsigned char * const screen_ram = (unsigned char*)0xCC00; // If using custom font
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
  assert(x < 40);
  assert(y < 25);
  c64_set_cursor_pos(x, y);
}

void put_screen(uint16_t linearPos, char ch, unsigned char color)
{
  assert(linearPos < 40*25);
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
  assert(linearPos < 40*25);
  put_screen(linearPos, ch, 7);
}

char get_screen(uint16_t linearPos)
{
  assert(linearPos < 40*25);
  // Gotcha: when reading the screen position where cursor
  // is currently at, must read the character under cursor instead.
  if (linearPos == c64_get_cursor_linear_pos()) return *char_under_cursor;
  return screen_ram[linearPos]; // TODO: screen code to PETSCII
}

void put_screen_color(uint16_t linearPos, unsigned char color)
{
  assert(linearPos < 40*25);
  put_screen(linearPos, get_screen(linearPos), 7);
}

unsigned char get_screen_color(uint16_t linearPos)
{
  assert(linearPos < 40*25);
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
  // Disabled due to custom programming font
//  *mem_setup_register = 0x17;

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

void scroll_screen_up(int8_t lines_scrolled)
{
  /*
  // Don't scroll first line since it contains the open files list - copy that manually
  char *ram_copy_start = (char*)screen_ram + SCREEN_WIDTH;
  char *color_copy_start = (char*)color_ram + SCREEN_WIDTH;
  int copy_size = (SCREEN_WIDTH-1)*SCREEN_HEIGHT-offset;
  memmove(ram_copy_start + offset, ram_copy_start, copy_size);
  memmove(color_copy_start + offset, color_copy_start, copy_size);
  */
  int end = SCREEN_WIDTH*SCREEN_HEIGHT;
  int offset = SCREEN_WIDTH*(int)lines_scrolled;
  char *ram_dst = (char*)screen_ram + end - SCREEN_WIDTH;
  char *ram_src = ram_dst - offset;
  char *col_dst = (char*)color_ram + end - SCREEN_WIDTH;
  char *col_src = col_dst - offset;
  for(int y = 0; y < SCREEN_HEIGHT-1-lines_scrolled; ++y)
  {
    memcpy8(ram_dst, ram_src, SCREEN_WIDTH);
    ram_dst -= SCREEN_WIDTH;
    ram_src -= SCREEN_WIDTH;
    memcpy8(col_dst, col_src, SCREEN_WIDTH);
    col_dst -= SCREEN_WIDTH;
    col_src -= SCREEN_WIDTH;
  }
}

void scroll_screen_down(int8_t lines_scrolled)
{
  /*
  // Don't scroll last line since it contains the line:column number info at bottom right
  char *ram_copy_start = (char*)screen_ram;
  char *color_copy_start = (char*)color_ram;
  int offset = SCREEN_WIDTH*(int)lines_scrolled;
  int copy_size = (SCREEN_WIDTH-1)*SCREEN_HEIGHT-offset;
  memmove(ram_copy_start, ram_copy_start + offset, copy_size);
  memmove(color_copy_start, color_copy_start + offset, copy_size);
  */
  int offset = SCREEN_WIDTH*(int)lines_scrolled;
  char *ram_dst = (char*)screen_ram + SCREEN_WIDTH;
  char *ram_src = ram_dst + offset;
  char *col_dst = (char*)color_ram + SCREEN_WIDTH;
  char *col_src = col_dst + offset;
  for(int y = 0; y < SCREEN_HEIGHT-2-lines_scrolled; ++y)
  {
    memcpy8(ram_dst, ram_src, SCREEN_WIDTH);
    ram_dst += SCREEN_WIDTH;
    ram_src += SCREEN_WIDTH;
    memcpy8(col_dst, col_src, SCREEN_WIDTH);
    col_dst += SCREEN_WIDTH;
    col_src += SCREEN_WIDTH;
  }
}
