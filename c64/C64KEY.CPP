#include "../input.h"

volatile unsigned char *key_address = (unsigned char *)0x00CB;
volatile unsigned char *key_modifiers = (unsigned char *)0x028D;

// https://www.c64-wiki.com/wiki/Zeropage: $0002 is unused so safe to access.
volatile unsigned char *tmp = (unsigned char *)0x0002; // arbitrary safe temporary memory address for getting data out of inline assembly

static volatile unsigned char * const jiffy_lsb = (unsigned char*)0xA2;

static void delay(int num_frames)
{
  unsigned char val = *jiffy_lsb;
  while(*jiffy_lsb - val < num_frames) /*nop*/;
}

static unsigned char key_is_repeating = 0;

unsigned char lastReturnedKeyCode = 0;
unsigned char lastKeyCode = 0;
unsigned char last_key_time = 0;
int get_key()
{
  unsigned char keyCode = 0, ch;
  while(!keyCode)
  {
    // https://sta.c64.org/cbm64krnfunc.html:
    // $FF9F == SCNKEY: "Query keyboard; put current matrix code into memory address $00CB, current status of shift keys into
    //                   memory address $028D and PETSCII code into keyboard buffer."
    asm volatile("JSR $FF9F\n"
    // $FFE4 == GETIN. Read byte from default input. (If not keyboard, must call OPEN and CHKIN beforehands.)
    //                 PETSCII code of read character will be in register A.
                 "JSR $FFE4\n"
                 "STA $0002\n"
                ::: "a", "x", "y", "c", "v", "memory");
    keyCode = *tmp;

    unsigned char repeat_delay = (key_is_repeating ? 10 : 15);
    if ((unsigned char)(*jiffy_lsb - last_key_time) >= repeat_delay) lastKeyCode = 0;

    if (keyCode == lastKeyCode && (unsigned char)(*jiffy_lsb - last_key_time) < repeat_delay)
      keyCode = 0;
  }
  key_is_repeating = (lastReturnedKeyCode == keyCode);
  lastKeyCode = keyCode;
  lastReturnedKeyCode = keyCode;
  last_key_time = *jiffy_lsb;

  // Is shift key down:
  unsigned char modifiers = *key_modifiers;
  bool is_shift = (modifiers&1);
//  bool is_cmdre = (modifiers&2);
//  bool is_ctrl = (modifiers&4);
  unsigned int key = keyCode;
  if (key == 0xA0) key = 0x20; // convert shift-space back to space.
  if (is_shift && is_shift_aware_key(key)) key |= KEY_SHIFT_MODIFIER;
  return key;
}

