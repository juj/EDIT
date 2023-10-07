#ifndef _MEMORY_H
#define _MEMORY_H

#if defined(__TURBOC__) && defined(__MSDOS__)
#include <alloc.h>
#endif

#include <stdlib.h>

#if !(defined(__TURBOC__) && defined(__MSDOS__))
#define far
#define huge
#define farmalloc malloc
#define _fmemcpy memcpy
#endif

extern size_t allocatedNearMemory;
extern size_t allocatedFarMemory;

void far *Farmalloc(size_t bytes);
void *Malloc(size_t bytes);

void Farfree(void far *ptr, size_t bytes);
void Free(void *ptr, size_t bytes);

#endif
