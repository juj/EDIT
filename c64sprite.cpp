// C64 sprite example with llvm-mos.
// Jukka Jylänki. Released to public domain.
#include <stdint.h>
#include <stdio.h>

static void TestAssert(bool condition, const char *str, const char *file, int line, const char *func)
{
#ifndef NDEBUG
  if (!condition) {
    printf("%s:%d:%s: ASSERT \"%s\" FAILED!\n", file, line, func, str);
    for(;;) { // Halt, and blink border red-black
      *(volatile uint8_t*)0xD020 = (*(volatile uint8_t*)0xD020) & 0xF ? 0 : 0x0A;
      for(unsigned int i = 0; i < 65535; ++i) asm("NOP":::"memory"); // pause for a moment
    }
  }
#endif
}
#define Assert(x) TestAssert((x), #x, __FILE__, __LINE__, __func__);

void SET_SPRITE_PTR(uint8_t i, void *ptr)
{
  Assert((uintptr_t)ptr < 16384 && (uintptr_t)ptr % 64 == 0);
  uint16_t sprite_ptr_addr = (((*(uint16_t*)0xD018) & 0xF0) << 6) + 1016;
  ((uint8_t*)sprite_ptr_addr)[i] = (uint8_t)((uintptr_t)ptr >> 6);
}

void ENABLE_SPRITE(uint8_t i) { *(uint8_t*)0xD015 |= (1<<i); }
void DISABLE_SPRITE(uint8_t i) { *(uint8_t*)0xD015 &= ~(1<<i); }
bool IS_SPRITE_ENABLED(uint8_t i) { return !!(*(uint8_t*)0xD015 & (1<<i)); }
void SET_SPRITE_COLOR(uint8_t i, uint8_t color) { ((uint8_t*)0xD027)[i] = color; }

void SET_SPRITE_POS(uint8_t i, uint16_t x, uint8_t y)
{
  ((uint8_t*)0xD000)[i<<1] = x;
  ((uint8_t*)0xD000)[(i<<1)+1] = y;
  if (x>>8) *((uint8_t*)0xD010) |= 1<<i;
  else *((uint8_t*)0xD010) &= ~(1<<i);
}

uint8_t sprite[64] __attribute__((aligned(64))) =
{
  0b00000000, 0b01010101, 0b00000000,
  0b00000010, 0b10101010, 0b10000000,
  0b00000101, 0b01010101, 0b01010000,
  0b00101010, 0b10101010, 0b10101000,
  0b01010101, 0b01010101, 0b01010101,
  0b10101010, 0b10101010, 0b10101010,
  0b01010101, 0b01010101, 0b01010101,
  0b10101010, 0b10101010, 0b10101010,
  0b01010101, 0b01010101, 0b01010101,
  0b10101010, 0b10101010, 0b10101010,
  0b01010101, 0b01010101, 0b01010101,
  0b10101010, 0b10101010, 0b10101010,
  0b01010101, 0b01010101, 0b01010101,
  0b10101010, 0b10101010, 0b10101010,
  0b01010101, 0b01010101, 0b01010101,
  0b10101010, 0b10101010, 0b10101010,
  0b01010101, 0b01010101, 0b01010101,
  0b00101010, 0b10101010, 0b10101000,
  0b00000101, 0b01010101, 0b01010000,
  0b00000010, 0b10101010, 0b10000000,
  0b00000000, 0b01010101, 0b00000000,
};

static const int8_t sincos[256] = { 0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48,51,54,57,59,62,65,67,70,73,75,78,80,82,85,87,89,91,94,96,98,100,102,103,105,107,108,110,112,113,114,116,117,118,119,120,121,122,123,123,124,125,125,126,126,126,126,126,127,126,126,126,126,126,125,125,124,123,123,122,121,120,119,118,117,116,114,113,112,110,108,107,105,103,102,100,98,96,94,91,89,87,85,82,80,78,75,73,70,67,65,62,59,57,54,51,48,45,42,39,36,33,30,27,24,21,18,15,12,9,6,3,0,-3,-6,-9,-12,-15,-18,-21,-24,-27,-30,-33,-36,-39,-42,-45,-48,-51,-54,-57,-59,-62,-65,-67,-70,-73,-75,-78,-80,-82,-85,-87,-89,-91,-94,-96,-98,-100,-102,-103,-105,-107,-108,-110,-112,-113,-114,-116,-117,-118,-119,-120,-121,-122,-123,-123,-124,-125,-125,-126,-126,-126,-126,-126,-127,-126,-126,-126,-126,-126,-125,-125,-124,-123,-123,-122,-121,-120,-119,-118,-117,-116,-114,-113,-112,-110,-108,-107,-105,-103,-102,-100,-98,-96,-94,-91,-89,-87,-85,-82,-80,-78,-75,-73,-70,-67,-65,-62,-59,-57,-54,-51,-48,-45,-42,-39,-36,-33,-30,-27,-24,-21,-18,-15,-12,-9,-6,-3, };

uint16_t RASTER_LINE() { return ((uint16_t)(*((volatile uint8_t*)0xD011) & 0x80) << 1) | *(volatile uint8_t*)0xD012; } // ghetto

int main()
{
  for(int i = 0; i < 8; ++i)
  {
    SET_SPRITE_PTR(i, sprite);
    ENABLE_SPRITE(i);
    SET_SPRITE_COLOR(i, i + (i>=6));
  }
  for(uint8_t i = 0;; ++i)
  {
    while(RASTER_LINE() < 200);
    for(int j = 0; j < 8; ++j)
      SET_SPRITE_POS(j, 170+(sincos[(uint8_t)(i+(j<<5))]>>1), 136+(sincos[(uint8_t)(i+64+(j<<5))]>>1));
    while(RASTER_LINE() > 200);
  }
}
