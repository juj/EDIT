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

static bool is_printable_character(uint8_t petsciiCode)
{
  if (petsciiCode == 0x5C) return false; // GBP character
  if (petsciiCode >= 0x20 && petsciiCode <= 0x5E) return true; // Petscii space, 0-9, a-z, and other characters
  if (petsciiCode >= 0xC1 && petsciiCode <= 0xDA) return true; // Petscii A-Z
  if (petsciiCode == PETSCII_THIN_UNDERSCORE) return true; // Underscore character '_'
  if (petsciiCode == PETSCII_LEFT_CURLY_BRACE) return true; // Petscii close substitute for ASCII '{'
  if (petsciiCode == PETSCII_BACKSLASH) return true; // Petscii close substitute for ASCII '\'
  if (petsciiCode == PETSCII_RIGHT_CURLY_BRACE) return true; // Petscii close substitute for ASCII '}'
  if (petsciiCode == PETSCII_OR_PIPE) return true; // Petscii graphical character for '|'
  return false;
}

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

  // Replace a thicker underscore character with the thinnest underscore to avoid causing
  // confusion which character represents an underscore symbol. (both can be used interchangeably)
  if (keyCode == PETSCII_THICK_UNDERSCORE) keyCode = PETSCII_THIN_UNDERSCORE;
  // 0xBA is Shift+@, which produces some uninteresting graphical character.
  // Replace it with underscore, which is Cmdr+@. (Shift+@ is easier to type than Cmdr+@)
  else if (keyCode == 0xBA) keyCode = PETSCII_THIN_UNDERSCORE;
  // This is somewhat of an arbitrary remap: Replace GBP character with backslash '\'
  else if (keyCode == PETSCII_GREAT_BRITAIN_POUND) keyCode = PETSCII_BACKSLASH;

  if (is_printable_character(keyCode)) return keyCode;
  unsigned char key_matrix_code = *key_address;
  return KEY_SCANCODE_MODIFIER | key_matrix_code | (((uint16_t)(*key_modifiers & 7)) << 8);
}

