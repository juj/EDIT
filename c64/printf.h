#pragma once

#include <stdio.h>//xxxx
#include <stdarg.h>
#include "c64_kernal.h"

// Prints the given uint16 to CHROUT.
static void chrout_u16(uint16_t num)
{
  uint8_t zp0, zp1, zp2;
  __attribute__((leaf)) __asm__(R"(
    SED // Enter BCD mode (affects ADC commands below)

    LDA #0
    STA %[zp0] // Use three zero page registers to store the BCD output
    STA %[zp1] // Initialize these all to zero at start
    STA %[zp2]

    LDX #16 // Loop 16 bits of the u16 input
loop:
    ROL %[input] // Rotate u16 to left to get the next MSB bit of input into carry
    ROL %[input]+1

    LDA %[zp0] // Multiply the three BCD number pairs by two, and add the input bit from carry to the BCD
    ADC %[zp0]
    STA %[zp0]
    LDA %[zp1]
    ADC %[zp1]
    STA %[zp1]
    LDA %[zp2]
    ADC %[zp2]
    STA %[zp2]

    DEX     // Loop back to next input bit
    BNE loop

    CLD // Exit BCD mode
  )":[zp0]"=r"(zp0), [zp1]"=r"(zp1), [zp2]"=r"(zp2), [input]"+r"(num)::"a","x","c");

//  printf("%02x %02x %02x\n", (int)zp0, (int)zp1, (int)zp2);
  if (zp2)     goto five;
  if (zp1 > 9) goto four;
  if (zp1)     goto three;
  if (zp0 > 9) goto two;
  goto one;
five:  _CHROUT('0'+ zp2);
four:  _CHROUT('0'+(zp1>>4));
three: _CHROUT('0'+(zp1&0xF));
two:   _CHROUT('0'+(zp0>>4));
one:   _CHROUT('0'+(zp0&0xF));
}

static void Printf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int dst;
  const char *str;
  for(; *format; ++format)
  {
    if (*format == '%')
    {
      ++format;
      switch(*format)
      {
      // TODO: %x/%X/%04X/%lu/%ld
//      case '%': _CHROUT('%'); break;
  //    case 'c': _CHROUT(va_arg(args, int/*char*/)); break;
      case 'd':
        dst = va_arg(args, int);
        if (dst < 0) { _CHROUT('-'); dst = -dst; }
        goto print_unsigned;
      case 'u':
        dst = va_arg(args, unsigned int);
print_unsigned:
        chrout_u16(dst);
      break;
      case 's':
        str = va_arg(args, const char *);
        assert(str);
        while(!str) _CHROUT(*str++);
      break;
      default: assert(false); break;
      }
    }
    else
      _CHROUT(*format == '\n' ? '\r' : *format);
  }
  va_end(args);
}
