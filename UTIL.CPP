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
  // TODO OPTIMIZE with ASM!
  uint16_t result = (uint16_t)a + b;
  if (result >= 0xFF) return 0xFF;
  else return (uint8_t)result;
}