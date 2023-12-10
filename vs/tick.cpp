#include "../c64/tick.h"
#include <Windows.h>
#include <stdio.h>

void tick_t::operator -=(const tick_t &rhs)
{
  value -= rhs.value;
}

void tick_t::print() const
{
  uint64_t freq = 0;
  QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
  double t = (double)value / freq;
  if (t < 1.0)
    printf("%f milliseconds.\n", t*1000.0);
  else
    printf("%f seconds.\n", t);
}

uint16_t tick_t::decisecs() const
{
  uint64_t freq = 0;
  QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
  return (uint16_t)(value * 10 / freq);
}

tick_t operator -(const tick_t &a, const tick_t &b)
{
  tick_t ret = a;
  ret -= b;
  return ret;
}

tick_t tick()
{
  uint64_t t;
  QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&t));
  return (tick_t){t};
}
