#pragma once

#include <stdint.h>

class tick_t
{
public:
#ifdef __mos__
  uint8_t tod[4];
#else
  uint64_t value;
#endif
  void operator -=(const tick_t &rhs);
  uint16_t decisecs() const;
  void print() const;
};

tick_t operator -(const tick_t &a, const tick_t &b);
tick_t tick(void);
