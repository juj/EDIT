#include "c64_kernal.h"
static volatile unsigned char * const screen_ram = (unsigned char*)0xCC00; // If using custom font
static volatile unsigned char * const char_under_cursor = (unsigned char *)0xCE;
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF;
static uint16_t cursorLinearPos;
extern uint8_t SCREEN_WIDTH;
void c64_set_cursor_pos(uint8_t x, uint8_t y)
{
  _DISABLE_INTR();
  if (*cursor_blink_phase) // If cursor is showing a reverse image right now, clear the reverse
  {
    screen_ram[cursorLinearPos] = *char_under_cursor;
  }
  cursorLinearPos = (uint16_t)y*SCREEN_WIDTH+x;
}
#include <stdio.h>
struct region
{
  region *next;
};
extern "C" char __heap_start;
static region *first_free_region = (region*)&__heap_start;
static region *unsorted_free_region = 0;
static bool linked_list_has_a_cycle(region *r)
{
}
extern "C" void malloc_defrag(uint8_t times)
{
  while(times--)
  {
    region *r = unsorted_free_region;
    if (!r) return;
    unsorted_free_region = r->next;
    {
      first_free_region = r;
    }
  }
}
extern "C" void *malloc(size_t bytes)
{
  for(region *r = first_free_region, *prev = 0; r; prev = r, r = r->next)
    {
      {
        region *newRegion = (region*)((uintptr_t)r + bytes);
        unsorted_free_region = newRegion;
      }
      return (void*)((uintptr_t)r + 2);
    }
}
extern "C" uint16_t malloc_free_heap_available()
{
}
extern "C" void free(void *ptr)
{
  if (!ptr) return;
  malloc_defrag(1);
}