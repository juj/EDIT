#include "../iassert.h"
#include "../screen.h"
#include "../input.h" // TODO REMOVE
#include <stdio.h>

static volatile unsigned char * const screen_mem = (unsigned char*)0x0400;

// Color RAM: https://www.c64-wiki.com/wiki/Color_RAM
static volatile unsigned char * const color_ram = (unsigned char*)0xD800;

int get_text_width()
{
  return 40;
}

int get_text_height()
{
  return 25;
}

static volatile unsigned char * const cursor_blink = (unsigned char *)0xCC;
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF;

#define CURSOR_NONE   0
#define CURSOR_NORMAL 1
#define CURSOR_INSERT 2
void set_cursor_style(int style)
{
  *cursor_blink = (style == CURSOR_NONE) ? 1 : 0;
}

static unsigned char cx, cy;
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
  screen_mem[cy*SCREEN_WIDTH+cx] = screen_mem[cy*SCREEN_WIDTH+cx] & 0x7F;
  cx = x;
  cy = y;
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
//  if (ch == 0xBA || ch == 0xFA) ch = 32;
//  if (ch < 32) screen_mem[linearPos] = 32;
  //else 
    screen_mem[linearPos] = petscii_to_screen_code(ch);

  color_ram[linearPos] = color;
  //screen_mem[linearPos] = (ch == 0) ? 32 : ch;
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
  SCREEN_WIDTH = get_text_width();
  SCREEN_HEIGHT = get_text_height();

  // Switch charset into text mode, where both lower case and upper case characters
  // are available.
  //*mem_setup_register ^= 2;
//  printf("%02XH\n", *mem_setup_register); // 0x15
  *mem_setup_register = 0x17;
//  printf("%02XH\n", *mem_setup_register); // 0x17
//  get_key();

  return true;
}

void deinit_screen()
{
}
