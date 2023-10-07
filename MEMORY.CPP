#include <conio.h>
#include "memory.h"
#include "audio.h"
#include "screen.h"
#include <dos.h>
#include <alloc.h>
#include <stdio.h>
#include <string.h>

unsigned long allocatedNearMemory = 0;
unsigned long allocatedFarMemory = 0;

void out_of_memory(int isFarMemory, unsigned long bytes)
{
  char str[64];
  sprintf(str, "Unable to allocate %lu bytes of %s memory!",
    bytes, isFarMemory ? "far" : "near");
  put_string_on_screen(str, (SCREEN_WIDTH - strlen(str))/2, SCREEN_HEIGHT-1,
    YELLOW | BGCOLOR(RED));
  ERROR_BEEP();
  getch();
}

void far *Farmalloc(unsigned long bytes)
{
  void far *ptr = farmalloc(bytes);
  if (ptr)
    allocatedFarMemory += bytes;
  else
    out_of_memory(1, bytes);
  return ptr;
}

void *Malloc(size_t bytes)
{
  void *ptr = malloc(bytes);
  if (ptr)
    allocatedNearMemory += bytes;
  else
    out_of_memory(1, bytes);
  return ptr;
}

void Farfree(void far *ptr, unsigned long bytes)
{
  if (ptr)
  {
    free(ptr);
    allocatedFarMemory -= bytes;
  }
}

void Free(void *ptr, size_t bytes)
{
  if (ptr)
  {
    free(ptr);
    allocatedNearMemory -= bytes;
  }
}
