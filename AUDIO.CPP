#include "audio.h"
#include <dos.h>

void beep(int frequency, int msecs)
{
  sound(frequency);
  delay(msecs);
  nosound();
}
