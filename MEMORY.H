#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdlib.h>

extern unsigned long allocatedNearMemory;
extern unsigned long allocatedFarMemory;

void far *Farmalloc(unsigned long bytes);
void *Malloc(size_t bytes);

void Farfree(void far *ptr, unsigned long bytes);
void Free(void *ptr, size_t bytes);

#endif
