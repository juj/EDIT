#include "../iassert.h"
#include "../screen.h"
#include "c64_kernal.h"
#include <stdio.h>
#include <stdint.h>

// Color RAM: https://www.c64-wiki.com/wiki/Color_RAM
static volatile unsigned char * const color_ram = (unsigned char*)0xD800;
static volatile unsigned char * const screen_mem = (unsigned char*)0x0400;
static volatile unsigned char * const cursor_blink = (unsigned char *)0xCC;
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF;
static volatile unsigned char * const cursor_color = (unsigned char*)0x0286;

#define CURSOR_NONE   0
#define CURSOR_NORMAL 1
#define CURSOR_INSERT 2
void set_cursor_style(uint8_t style)
{
  *cursor_blink = (style == CURSOR_NONE) ? 1 : 0;
}

static unsigned char cx, cy;
unsigned char colorUnderCursor = 7;
void set_cursor_pos(uint8_t x, uint8_t y)
{
  _PLOT_SET(x, y);
  // After changing cursor position, we need to clean up the old character, which
  // might have been left inverted by the blinking cursor in the old location
  uint16_t pos = (uint16_t)cy*SCREEN_WIDTH+cx;
  screen_mem[pos] = screen_mem[pos] & 0x7F;
  //color_ram[pos] = colorUnderCursor;
  cx = x;
  cy = y;
  //colorUnderCursor = *cursor_color = color_ram[y*SCREEN_WIDTH+x];
  *cursor_color = color_ram[y*SCREEN_WIDTH+x];
}

unsigned char petscii_to_screen_code(unsigned char petscii);
/*
{
  if (petscii <= 31) return 32;//petscii + 128;
  if (petscii <= 63) return petscii;
  if (petscii <= 95) return petscii - 64;
  if (petscii <= 127) return petscii - 32;
  if (petscii <= 159) return 32;//petscii + 64;
  if (petscii <= 191) return petscii - 64;
  if (petscii <= 254) return petscii - 128;
  return 0x5E;
}
*/

void put_screen(uint16_t linearPos, char ch, unsigned char color)
{
  screen_mem[linearPos] = petscii_to_screen_code(ch) ^ (color&0x80);
  color_ram[linearPos] = color;
}

void put_screen(uint16_t linearPos, char ch)
{
  put_screen(linearPos, ch, 7);
}

char get_screen(uint16_t linearPos)
{
  return screen_mem[linearPos]; // TODO: screen code to PETSCII
}

void put_screen_color(uint16_t linearPos, unsigned char color)
{
  put_screen(linearPos, get_screen(linearPos), 7);
}

unsigned char get_screen_color(uint16_t linearPos)
{
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
  *screen_frame = scrolled ? 6 : 14;
}
