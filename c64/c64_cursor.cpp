#include "c64_cursor.h"
#include "c64_kernal.h"

static volatile unsigned char * const color_ram = (unsigned char*)0xD800;
//static volatile unsigned char * const screen_ram = (unsigned char*)0x0400;
static volatile unsigned char * const screen_ram = (unsigned char*)0xCC00; // If using custom font
static volatile unsigned char * const cursor_blink = (unsigned char *)0xCC;
static volatile unsigned char * const cursor_blink_delay = (unsigned char *)0xCD;
static volatile unsigned char * const char_under_cursor = (unsigned char *)0xCE;
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF;
static volatile unsigned char * const cursor_color = (unsigned char*)0x0286;
static volatile unsigned char * const color_under_cursor = (unsigned char*)0x0287;

static uint16_t cursorLinearPos;

extern uint8_t SCREEN_WIDTH;

void c64_set_cursor_style(uint8_t style)
{
  *cursor_blink = (style == CURSOR_NONE) ? 1 : 0;
}

void c64_set_cursor_pos(uint8_t x, uint8_t y)
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

uint16_t c64_get_cursor_linear_pos()
{
  return cursorLinearPos;
}
