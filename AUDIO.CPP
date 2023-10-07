#include "audio.h"
#if defined(__TURBOC__) && defined(__MSDOS__)
#include <dos.h>
#endif

void beep(int frequency, int msecs)
{
#if defined(__TURBOC__) && defined(__MSDOS__)
  sound(frequency);
  delay(msecs);
  nosound();
#endif
}
