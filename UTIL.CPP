#if defined(_MSC_VER) || defined(__TURBOC__)
#include <conio.h>
#endif
#include "util.h"
#include "audio.h"
#include "screen.h"
#include "input.h"

void repaint_screen();

void NOTIMPL(const char *x)
{
  log_x(x);
  NOTIMPL_BEEP();
  get_key();
  repaint_screen();
}

uint8_t ADD_U8_SAT(uint8_t a, uint8_t b)
{
  /*
  uint8_t tmp, result;
  __asm__(R"(
    STX %[zp]
    CLC
    ADC %[zp]
    BCC good
    LDA #255
  good:
  )":"=a"(result):[zp]"r"(tmp),"a"(a),"x"(b):"c");
  return result;
  */
// This C code is verified to essentially emit
// the above assembly code.
  uint8_t result = a + b;
  if (result < a) return 0xFF;
  else return result;
}

uint8_t SUB_U8_SAT(uint8_t a, uint8_t b)
{
#if defined(__C64__) && 0 // TODO: Currently disabled, as this results in incorrect code somehow?
  uint8_t tmp, result;
  __attribute__((leaf)) __asm__(R"(
    STX %[zp]
    SEC
    SBC %[zp]
    BCS good
    LDA #0
  good:
  )":"=a"(result),[zp]"+r"(tmp):"a"(a),"x"(b):"c");
  return result;
#else
// The above code is an optimized form of this code:
  if (a < b) return 0;
  return a - b;
#endif
}
