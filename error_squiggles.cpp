#include <stdint.h>
#include "error_squiggles.h"
#include "state.h"
#include "screen.h"

void ENABLE_SPRITE(uint8_t i) { *(uint8_t*)0xD015 |= (1<<i); } // Show i'th sprite
void DISABLE_SPRITE(uint8_t i) { *(uint8_t*)0xD015 &= ~(1<<i); } // Hide i'th sprite
bool IS_SPRITE_ENABLED(uint8_t i) { return !!(*(uint8_t*)0xD015 & (1<<i)); }
void SET_SPRITE_COLOR(uint8_t i, uint8_t color) { ((uint8_t*)0xD027)[i] = color; } // Color [0,15]

void SET_SPRITE_POS(uint8_t i, uint16_t x, uint8_t y) // X=[0, 511], Y=[0,255]
{
  ((uint8_t*)0xD000)[i<<1] = x; // Set bits 0-7 of x coordinate
  ((uint8_t*)0xD000)[(i<<1)+1] = y; // Set y coordinate
  if (x>>8) *((uint8_t*)0xD010) |= 1<<i; // Set bit 8 of x coordinate in overflow memory
  else *((uint8_t*)0xD010) &= ~(1<<i); // ...or clear bit 8 of x coordinate in overflow memory
}

static line_num squiggles_line_num;
static line_xpos squiggles_column;
static uint8_t squiggles_type; // 0 - red, 1 - build ok (squiggles not visible), 2 - yellow

void set_error_squiggles(line_num line_num_, line_xpos column_, uint8_t type_)
{
  squiggles_line_num = line_num_;
  squiggles_column = column_;
  squiggles_type = type_;

  if (squiggles_type == 1)
    DISABLE_SPRITE(0);
  else
  {
    update_error_squiggles_pos();
    SET_SPRITE_COLOR(0, (squiggles_type == 2 ? YELLOW : LTRED));
    ENABLE_SPRITE(0);
  }
}

void update_error_squiggles_pos()
{
  if (squiggles_type != 1)
    SET_SPRITE_POS(0, (uint16_t)(squiggles_column-viewportXOffset)*8 + 16, (squiggles_line_num-firstDisplayedLineNumber)*8 + 48);
}
