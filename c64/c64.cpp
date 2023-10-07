#include "../iassert.h"
#include "../screen.h"
#include <stdio.h>

unsigned char *screen_mem = (unsigned char*)0x0400;

// Color RAM: https://www.c64-wiki.com/wiki/Color_RAM
unsigned char *color_ram = (unsigned char*)0xD800;

int get_text_width()
{
  return 40;
}

int get_text_height()
{
  return 25;
}

volatile unsigned char *cursor_blink = (unsigned char *)0x00CC;

void set_cursor_pos(int x, int y)
{
  unsigned char prev_blink = *cursor_blink;
  *cursor_blink = 1; // disable cursor temporarily
    // https://sta.c64.org/cbm64krnfunc.html:
    // $E50A == Save (get) or restore (set) cursor position.
    asm volatile("CLC\n" // Clear Carry: Carry flag 0 signals to the KERNAL subroutine that it should set the cursor position
                 "JSR $E50A\n"
                :: "x"((unsigned char)y), "y"((unsigned char)x) : "a", "x", "y", "memory");
  *cursor_blink = prev_blink;
}

#define CURSOR_NONE   0
#define CURSOR_NORMAL 1
#define CURSOR_INSERT 2
void set_cursor_style(int style)
{
  *cursor_blink = (style == CURSOR_NONE) ? 1 : 0;
}

void put_screen(int linearPos, char ch, unsigned char color)
{
  if (ch == 0xBA || ch == 0xFA) ch = 32;
  if (ch < 32) screen_mem[linearPos] = 32;
  else screen_mem[linearPos] = ch;

  color_ram[linearPos] = color;
  //screen_mem[linearPos] = (ch == 0) ? 32 : ch;
}

void put_screen(int linearPos, char ch)
{
  put_screen(linearPos, ch, 7);
}

char get_screen(int linearPos)
{
  return screen_mem[linearPos];
}

void put_screen_color(int linearPos, unsigned color)
{
  put_screen(linearPos, get_screen(linearPos), 7);
}

unsigned char get_screen_color(int linearPos)
{
  return 0; // TODO
}

int init_screen()
{
  SCREEN_WIDTH = get_text_width();
  SCREEN_HEIGHT = get_text_height();

  return true;
}

void deinit_screen()
{
}
