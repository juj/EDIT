#include "tick.h"
#include "c64_kernal.h"

// An array of four uint8's:
// time_of_day[0]: 1/10ths of seconds, in BCD, i.e. in range 0-9.
// time_of_day[1]: seconds, in BCD, i.e. in range $00-$59.
// time_of_day[2]: minutes, in BCD, i.e. in range $00-$59.
// time_of_day[3]: bits 0-5: hours, in BCD, i.e. in range $00-$12.
// time_of_day[3]: bit 7: 0=AM, 1=PM.
static volatile uint8_t * const time_of_day = (uint8_t*)0xDD08;
static volatile uint8_t * const timer_a_ctrl = (uint8_t*)0xDD0E;
static volatile uint8_t * const timer_b_ctrl = (uint8_t*)0xDD0F;
static volatile uint16_t * const timer_a_value = (uint16_t*)0xDD04;

static const char hex[] = "0123456789ABCDEF";

extern "C" void sub_tick_t(uint8_t *a, const uint8_t *b);

__attribute__((constructor(100)))
static void initialize_tod()
{
  *time_of_day = 0; // Writing 1/10ths of seconds starts the TOD clock.
}

void tick_t::operator -=(const tick_t &rhs)
{
  sub_tick_t(tod, rhs.tod);
}

void tick_t::print() const
{
  if (tod[3]) goto hours;
  if (tod[2]) goto minutes;
  goto seconds;
hours:
  if (tod[3]>>4) _CHROUT(hex[tod[3]>>4]);
  _CHROUT(hex[tod[3]&15]);
  _CHROUT('H');
  _CHROUT(' ');
minutes:
  if (tod[2]>>4) _CHROUT(hex[tod[2]>>4]);
  _CHROUT(hex[tod[2]&15]);
  _CHROUT('M');
  _CHROUT(' ');
seconds:
  if (tod[1]>>4) _CHROUT(hex[tod[1]>>4]);
  _CHROUT(hex[tod[1]&15]);
  _CHROUT('.');
  _CHROUT(hex[tod[0]&15]);
  _CHROUT('S');
  _CHROUT('\r');
}

uint16_t tick_t::decisecs() const
{
  int h = (tod[3]>>4)*10+(tod[3]&15);
  int m = (tod[2]>>4)*10+(tod[2]&15);
  int s = (tod[1]>>4)*10+(tod[1]&15);
  return ((h*60+m)*60+s)*10+tod[0];
}

tick_t operator -(const tick_t &a, const tick_t &b)
{
  tick_t ret = a;
  ret -= b;
  return ret;
}

tick_t tick()
{
  // Reading hours latches the TOD clock update values, so that the subsequent 
  // min:sec:1/10ths of sec regs can be read without fear of a race condition/unsynchronized
  // data being received.
  uint8_t h = time_of_day[3];
  // TOD clock reports hour digits 1-12. Convert digit 12 back to zero for
  // 0-11 values, which is more straightforward for modular arithmetic.
  if ((h&0x7F) == 0x12) h = h & 0x80;
  // Convert 12h AM/PM clock to 24:00 format and clear the AM/PM bit.
  if ((h&0x80) != 0) h = (h & 0x7F) + 0x12;
  uint8_t m = time_of_day[2];
  uint8_t s = time_of_day[1];
  uint8_t s10 = time_of_day[0];
  return (tick_t) { s10, s, m, h };
}

/*
void chrout_tod()
{
  // Reading hours latches the TOD clock update values, so that the subsequent 
  // min:sec:1/10ths of sec regs can be read without fear of a race condition/unsynchronized
  // data being received.
  uint8_t h = time_of_day[3];
  // TOD clock reports hour digits 1-12. Convert digit 12 back to zero for
  // 0-11 values, which is more straightforward for modular arithmetic.
  if ((h&0x7F) == 0x12) h = h & 0x80;
  // Convert 12h AM/PM clock to 24:00 format and clear the AM/PM bit.
  if ((h&0x80) != 0) h = (h & 0x7F) + 0x12;

  _CHROUT(hex[(h>>4)&7]);
  _CHROUT(hex[h&15]);
  _CHROUT('.');
  uint8_t m = time_of_day[2];
  _CHROUT(hex[m>>4]);
  _CHROUT(hex[m&15]);
  _CHROUT('.');
  uint8_t s = time_of_day[1];
  _CHROUT(hex[s>>4]);
  _CHROUT(hex[s&15]);
  _CHROUT(':');
  // Reading 1/10ths of seconds register resumes the TOD clock updates.
  _CHROUT(hex[time_of_day[0]&15]);
  _CHROUT('\r');
}
*/
