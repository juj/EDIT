#ifndef _AUDIO_H
#define _AUDIO_H

void beep(int frequency, int msecs);

#define ERROR_BEEP() beep(170, 50)

#define NOTIMPL_BEEP() do { beep(1000, 50); beep(700, 50); beep(1000, 50); } while(0)

#endif
