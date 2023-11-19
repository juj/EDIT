#include "c64/c64_kernal.h"
#include <stdio.h>

int main()
{
  for(;;)
  {
    _Jiffies j = _RDTIM();
    uint8_t hi = j >> 16;
    uint8_t md = (j >> 8) & 255;
    uint8_t lo = j & 255;
    printf("%lu %x %x %x\n", (unsigned long)j, hi, md, lo);
  }
}
