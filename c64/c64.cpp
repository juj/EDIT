#include "../iassert.h"
#include "../screen.h"
#include <stdio.h>

// Color RAM: https://www.c64-wiki.com/wiki/Color_RAM
static volatile unsigned char * const color_ram = (unsigned char*)0xD800;
static volatile unsigned char * const screen_mem = (unsigned char*)0x0400;
static volatile unsigned char * const cursor_blink = (unsigned char *)0xCC;
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF;
static volatile unsigned char * const cursor_color = (unsigned char*)0x0286;

#define CURSOR_NONE   0
#define CURSOR_NORMAL 1
#define CURSOR_INSERT 2
void set_cursor_style(int style)
{
  *cursor_blink = (style == CURSOR_NONE) ? 1 : 0;
}

static unsigned char cx, cy;
unsigned char colorUnderCursor = 7;
void set_cursor_pos(int x, int y)
{
  // https://sta.c64.org/cbm64krnfunc.html:
  // $E50A == Save (get) or restore (set) cursor position.
  // Disassembly at https://skoolkid.github.io/sk6502/c64rom/asm/E50A.html
  asm volatile(R"(CLC // Clear Carry: Carry flag 0 signals to the KERNAL subroutine that it should set the cursor position
                  JSR $E50A)"
              :: "x"((unsigned char)y), "y"((unsigned char)x) : "a", "c", "memory");

  // After changing cursor position, we need to clean up the old character, which
  // might have been left inverted by the blinking cursor in the old location
  int pos = cy*SCREEN_WIDTH+cx;
  screen_mem[pos] = screen_mem[pos] & 0x7F;
  //color_ram[pos] = colorUnderCursor;
  cx = x;
  cy = y;
  //colorUnderCursor = *cursor_color = color_ram[y*SCREEN_WIDTH+x];
  *cursor_color = color_ram[y*SCREEN_WIDTH+x];
}

unsigned char petscii_to_screen_code(unsigned char petscii)
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

void put_screen(int linearPos, char ch, unsigned char color)
{
  screen_mem[linearPos] = petscii_to_screen_code(ch) ^ (color&0x80);
  color_ram[linearPos] = color;
}

void put_screen(int linearPos, char ch)
{
  put_screen(linearPos, ch, 7);
}

char get_screen(int linearPos)
{
  return screen_mem[linearPos]; // TODO: screen code to PETSCII
}

void put_screen_color(int linearPos, unsigned color)
{
  put_screen(linearPos, get_screen(linearPos), 7);
}

unsigned char get_screen_color(int linearPos)
{
  return color_ram[linearPos];
}

static volatile unsigned char * const mem_setup_register = (unsigned char *)0xD018;

int init_screen()
{
  unsigned char w,h;
  asm("JSR 0xE505"
    :"=x"(w), "=y"(h));
  SCREEN_WIDTH = w;
  SCREEN_HEIGHT = h;

  // Switch charset into text mode, where both lower case and upper case characters
  // are available. Default register value at boot is 0x15.
  *mem_setup_register = 0x17;

  return true;
}

void deinit_screen()
{
}
