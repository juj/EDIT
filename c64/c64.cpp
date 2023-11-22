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
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF; // 0: cursor off phase, 1: cursor on (rev char)
static volatile unsigned char * const cursor_color = (unsigned char*)0x0286;
static volatile unsigned char * const color_under_cursor = (unsigned char*)0x0287;

unsigned char petscii_to_screen_code(unsigned char petscii);

unsigned char screen_code_to_petscii(unsigned char screencode)
{
  screencode &= 0x7F;
  if (32 <= screencode && screencode <= 63) return screencode;
  if (screencode <= 95) return screencode + 32;
  return screencode + 64;
  /*
  if (petscii <= 31) return 32;//petscii + 128;
  if (petscii <= 63) return petscii;
  if (petscii <= 95) return petscii - 64;
  if (petscii <= 127) return petscii - 32;
  if (petscii <= 159) return 32;//petscii + 64;
  if (petscii <= 191) return petscii - 64;
  if (petscii <= 254) return petscii - 128;
  return 0x5E;
  */

}

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

void temp_hide_cursor()
{
  *cursor_blink_delay = 20;
  *cursor_blink_phase = 0;
  screen_ram[c64_get_cursor_linear_pos()] &= 0x7F;
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
  return screen_code_to_petscii(screen_ram[linearPos]);
}

void put_screen_color(uint16_t linearPos, unsigned char color)
{
  assert(linearPos < 40*25);
  if (linearPos == c64_get_cursor_linear_pos()) put_screen(linearPos, get_screen(linearPos), color);
  else color_ram[linearPos] = color;
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

void scroll_screen_left(int8_t chars_scrolled)
{
  assert(chars_scrolled > 0);
  assert(chars_scrolled < SCREEN_WIDTH);
  int offset = chars_scrolled;
  char *ram_src = (char*)screen_ram;
  char *ram_dst = ram_src + offset;
  char *col_src = (char*)color_ram;
  char *col_dst = col_src + offset;
  _DISABLE_INTR();
  temp_hide_cursor();
  for(int8_t y = 0; y < SCREEN_HEIGHT; ++y)
  {
    memmove(ram_dst, ram_src, SCREEN_WIDTH - chars_scrolled);
    ram_dst += SCREEN_WIDTH;
    ram_src += SCREEN_WIDTH;
    memmove(col_dst, col_src, SCREEN_WIDTH - chars_scrolled);
    col_dst += SCREEN_WIDTH;
    col_src += SCREEN_WIDTH;
  }  
  _ENABLE_INTR();
}

void scroll_screen_right(int8_t chars_scrolled)
{
  assert(chars_scrolled > 0);
  assert(chars_scrolled < SCREEN_WIDTH);
  int offset = chars_scrolled;
  char *ram_dst = (char*)screen_ram;
  char *ram_src = ram_dst + offset;
  char *col_dst = (char*)color_ram;
  char *col_src = col_dst + offset;
  _DISABLE_INTR();
  temp_hide_cursor();
  for(int y = 0; y < SCREEN_HEIGHT; ++y)
  {
    memcpy8(ram_dst, ram_src, SCREEN_WIDTH - chars_scrolled);
    ram_dst += SCREEN_WIDTH;
    ram_src += SCREEN_WIDTH;
    memcpy8(col_dst, col_src, SCREEN_WIDTH - chars_scrolled);
    col_dst += SCREEN_WIDTH;
    col_src += SCREEN_WIDTH;
  }  
  _ENABLE_INTR();
}