#include "../input.h"
#include "c64_kernal.h"

static volatile unsigned char * const queued_key_buffer_length = (unsigned char *)0x00C6;

static volatile unsigned char * const key_matrix_code = (unsigned char *)0x00CB;
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

static bool is_printable_petscii(uint8_t petsciiCode)
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

bool is_printable_key(int key)
{
  return (key & KEY_SCANCODE_MODIFIER) == 0;
}

int get_key()
{
  unsigned char keyCode = 0, ch;
  while(!keyCode)
    keyCode = _GETIN();
  uint8_t modifiers = *key_modifiers & 7; // bit 0: shift, bit 1: Commodore key, bit 2: Ctrl

  // Replace a thicker underscore character with the thinnest underscore to avoid causing
  // confusion which character represents an underscore symbol. (both can be used interchangeably)
  if (keyCode == PETSCII_THICK_UNDERSCORE) keyCode = PETSCII_THIN_UNDERSCORE;
  // 0xBA is Shift+@, which produces some uninteresting graphical character.
  // Replace it with underscore, which is Cmdr+@. (Shift+@ is easier to type than Cmdr+@)
  else if (keyCode == 0xBA) keyCode = PETSCII_THIN_UNDERSCORE;
  // This is somewhat of an arbitrary remap: Replace GBP character with backslash '\'
  else if (keyCode == PETSCII_GREAT_BRITAIN_POUND) keyCode = PETSCII_BACKSLASH;
  else if (keyCode == 0xC0) keyCode = PETSCII_LEFT_CURLY_BRACE; // Shift-* -> '{'
  else if (keyCode == 0xDE) keyCode = PETSCII_RIGHT_CURLY_BRACE; // Shift-UpArrow -> '}'
  else if (keyCode == 0xA0 && modifiers == 1) keyCode = 0x20; // Treat Shift-Space as just Space since it eases typing all uppercase text
  else if ((keyCode == 0x3C || keyCode == 0x3E) && (modifiers & 2)) keyCode = 0; // Cmdr-, and Cmdr-. indent and unindent
  else if (keyCode == PETSCII_EQUALS && modifiers) keyCode = 0; // Do not treat Shift-= and Cmdr-= as printable

  if (is_printable_petscii(keyCode)) return keyCode;
  return KEY_SCANCODE_MODIFIER | *key_matrix_code | (((uint16_t)modifiers) << 8);
}

