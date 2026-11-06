#include <stdint.h>

 #define _ASM __attribute__((leaf)) __asm__
 #define _KERNAL static __inline__ __attribute__((__always_inline__, __nodebug__, leaf))
//#define _ASM __asm__
//#define _KERNAL static __inline__ __attribute__((__always_inline__, __nodebug__))

_KERNAL void _DISABLE_INTR() { _ASM volatile("SEI"); }

// Re-enables interrupts.
_KERNAL void _ENABLE_INTR() { _ASM volatile("CLI"); }

static uint16_t cursorLinearPos;

static volatile unsigned char * const cursor_blink_delay = (unsigned char *)0xCD;
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF;
static volatile unsigned char * const screen_ram = (unsigned char*)0xCC00; // If using custom font
static volatile unsigned char * const color_ram = (unsigned char*)0xD800;

uint8_t SCREEN_WIDTH = 40;
uint8_t SCREEN_HEIGHT = 25;

uint16_t c64_get_cursor_linear_pos()
{
  return cursorLinearPos;
}

void temp_hide_cursor()
{
  *cursor_blink_delay = 20;
  *cursor_blink_phase = 0;
  screen_ram[c64_get_cursor_linear_pos()] &= 0x7F;
}

extern "C" void *memcpy8(void *dest, const void *src, uint8_t count) // TODO: optimize
{
//  assert((uintptr_t)dest >= 0x2);
//  assert((uintptr_t)src >= 0x2);
  char *d = (char*)dest;
  const char *s = (const char *)src;
  while(count--) *d++ = *s++;
  return dest;
}

void scroll_screen_right(int8_t chars_scrolled)
{
//  assert(chars_scrolled > 0);
//  assert(chars_scrolled < SCREEN_WIDTH);
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

int main()
{
  scroll_screen_right(*(volatile unsigned char*)0xD020);
  scroll_screen_right(*(volatile unsigned char*)0xD021);
}
