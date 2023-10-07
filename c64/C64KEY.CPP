#include "../input.h"

volatile unsigned char *key_address = (unsigned char *)0x00CB;
volatile unsigned char *key_modifiers = (unsigned char *)0x028D;
volatile unsigned char *tmp = (unsigned char *)0x0080; // arbitrary temporary memory address for getting data out of inline assembly

int get_key()
{
  unsigned char keyCode = 0;
  while(!keyCode)
  {
    // https://sta.c64.org/cbm64krnfunc.html:
    // $FF9F == SCNKEY: "Query keyboard; put current matrix code into memory address $00CB, current status of shift keys into
    //                   memory address $028D and PETSCII code into keyboard buffer."
    asm volatile("JSR $FF9F\n"
    // $FFE4 == GETIN. Read byte from default input. (If not keyboard, must call OPEN and CHKIN beforehands.)
    //                 PETSCII code of read character will be in register A.
                 "JSR $FFE4\n"
                 "STA $0080\n"
                ::: "a", "x", "y", "memory");
    keyCode = *tmp;
  }

  // Is shift key down:
  unsigned char modifiers = *key_modifiers;
  bool is_shift = (modifiers&1);
  bool is_cmdre = (modifiers&2);
  bool is_ctrl = (modifiers&4);
  unsigned int key = keyCode;
  if (key == 0xA0) key = 0x20; // convert shift-space back to space.
  if (is_shift && is_shift_aware_key(key)) key |= KEY_SHIFT_MODIFIER;
  return key;
}
