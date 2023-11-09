#if defined(_MSC_VER) || defined(__TURBOC__)
#include <conio.h>
#include <dos.h>
#endif
#include "memory.h"
#include "audio.h"
#include "screen.h"
#include "input.h"
#include <stdio.h>
#include <string.h>

size_t allocatedNearMemory = 0;
size_t allocatedFarMemory = 0;

void repaint_status_line();

void out_of_memory(int isFarMemory, size_t bytes)
{
  char str[64];
#if defined(_MSC_VER) || defined(__TURBOC__)
  sprintf(str, "Unable to allocate %zu bytes of %s memory!",
    bytes, isFarMemory ? "far" : "near");
#else

#ifndef __C64__ // TODO SPRINTF
  sprintf(str, "OOM(%u BYTES)", bytes);
#else
  strcpy(str, "OOM");
#endif

#endif
  put_string_on_screen(str, (SCREEN_WIDTH - (int)strlen(str))/2, SCREEN_HEIGHT-1,
    YELLOW | BGCOLOR(RED));
  ERROR_BEEP();
  get_key();
  repaint_status_line();
}

void far *Farmalloc(size_t bytes)
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

void Farfree(void far *ptr, size_t bytes)
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
