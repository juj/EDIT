#include "../input.h"
#include "c64_kernal.h"

static volatile unsigned char * const queued_key_buffer_length = (unsigned char *)0x00C6;

static volatile unsigned char * const key_address = (unsigned char *)0x00CB;
static volatile unsigned char * const key_modifiers = (unsigned char *)0x028D;

static volatile unsigned char * const jiffy_lsb = (unsigned char*)0xA2;

static void delay(int num_frames)
{
  unsigned char val = *jiffy_lsb;
  while(*jiffy_lsb - val < num_frames) /*nop*/;
}

static unsigned char key_is_repeating = 0;

bool is_shift_key_currently_down()
{
  return *key_modifiers & 1;
}

bool is_ctrl_key_currently_down()
{
  return *key_modifiers & 4;
}

bool is_alt_key_currently_down()
{
  return *key_modifiers & 2;
}

bool is_get_key_pending()
{
  return *queued_key_buffer_length != 0;
}

unsigned char lastReturnedKeyCode = 0;
unsigned char lastKeyCode = 0;
unsigned char last_key_time = 0;

int get_key()
{
  unsigned char keyCode = 0, ch;
  while(!keyCode)
  {
    keyCode = _GETIN();

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
  bool is_ctrl = (modifiers&4);
  unsigned int key = keyCode;
  unsigned char key_matrix_code = *key_address;

  if (key >= 0xC1 && key <= 0xDA) key -= 0x60; // xxx why this?
//  key &= 0x7F; // xxx why this?
  if (keyCode == 0xBA && key_matrix_code == 0x2E) key = LEFT_CURLY_BRACE;
  else if (keyCode == 0xC0 && key_matrix_code == 0x31) key = RIGHT_CURLY_BRACE;
  else if (keyCode == 0xDE && key_matrix_code == 0x36) key = TEXT_BACKSLASH;
  else if (key == 0xA0) key = 0x20; // convert shift-space back to space. xxx why this?

  if (is_shift)
  {
    if (is_shift_aware_key(key)) key |= KEY_SHIFT_MODIFIER;
    else if (key >= 'A' && key <= 'Z')
      key ^= 0x20; // In PETSCII uppercase and lowercase are reversed
  }
  if (key == 0x5F) // The PETSCII back arrow key at top left of keyboard
  {
    if (is_ctrl) key = KEY_CTRL_TAB;
    else key = KEY_TAB;
  }
//  printf("  %XH/%XH\n", (int)key, (int)*key_address);
  return key;
}

