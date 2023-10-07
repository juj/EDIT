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
