#include <stdio.h>
#include <stdint.h>
#include "c64/c64_kernal.h"
#include "c64/tick.h"

int main()
{
  for(int i = 0; i < 111; ++i) printf("TEST LINE %d\n", i);

  tick_t t0 = tick();
  for(int i = 0; i < 152; ++i) printf("TEST LINE %d\n", i);
  tick_t t1 = tick();
  t0.print();
  t1.print();
  tick_t t2 = t1 - t0;
  t2.print();
}
