#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

volatile uint16_t *sid_voice_3_frequency = (uint16_t*)0xD40E;
volatile uint8_t *sid_voice_3_control_register = (uint8_t*)0xD412;
volatile uint8_t *sid_rng = (uint8_t*)0xD41B;

extern "C" void malloc_dump_heap();

int main()
{
  *sid_voice_3_frequency = 0xFFFF;
  *sid_voice_3_control_register = 0x80;
  int used = 0;
  char **ptrs = (char**)malloc(256*sizeof(char*));
/*
  malloc_dump_heap();
  malloc_dump_heap();
  printf("0X%X\n", malloc(512));
  malloc_dump_heap();
  void *ptr = malloc(253);
  malloc_dump_heap();
  printf("0X%X\n", ptr);
  free(ptr);
  malloc_dump_heap();
  printf("0X%X\n", malloc(106));
  malloc_dump_heap();
*/
  for(;;)//int i = 0; i < 100; ++i)
  {
    uint8_t rng = *sid_rng;
    if (rng%2 && used < 256)
    {
      ptrs[used] = (char*)malloc(*sid_rng);
      printf("MALLOC: 0X%X\n", ptrs[used]);
      assert(ptrs[used]);
      ++used;
    }
    else if (used > 0)
    {
      uint8_t free_idx = *sid_rng % used;
      free(ptrs[free_idx]);
      printf("FREE: 0X%X\n", ptrs[free_idx]);
      ptrs[free_idx] = ptrs[--used];
    }
  }
}
