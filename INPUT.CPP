#include "input.h"
#include <conio.h>
#include <dos.h>

#define SCANCODE_LEFT_ALT 0x38
#define SCANCODE_RIGHT_ALT 0x138
#define SCANCODE_LEFT_SHIFT 0x2A
#define SCANCODE_RIGHT_SHIFT 0x36
#define SCANCODE_LEFT_CTRL 0x1D
#define SCANCODE_RIGHT_CTRL 0x11D

unsigned char leftAltDown = 0, rightAltDown = 0;
unsigned char leftShiftDown = 0, rightShiftDown = 0;
unsigned char leftCtrlDown = 0, rightCtrlDown = 0;

unsigned short pendingScanCode;

static void _far _interrupt (*oldKbHandler)(...);

static void _far _interrupt key_interrupt(...)
{
  static unsigned short extended;
  unsigned char code = inportb(0x60);
  /*
  unsigned char temp = inportb(0x61);
  outportb(0x61, temp | 0x80);
  outportb(0x61, temp);
  */
  if (code == 0xe0)
  {
    extended = 0x100;
  }
  else
  {
    unsigned short scanCode = extended | (code & 0x7f);
    extended = 0;
    unsigned char keyDown = (code >> 7) ^ 1;
    if (scanCode == SCANCODE_LEFT_ALT) leftAltDown = keyDown;
    else if (scanCode == SCANCODE_RIGHT_ALT) rightAltDown = keyDown;
    else if (scanCode == SCANCODE_LEFT_SHIFT) leftShiftDown = keyDown;
    else if (scanCode == SCANCODE_RIGHT_SHIFT) rightShiftDown = keyDown;
    else if (scanCode == SCANCODE_LEFT_CTRL) leftCtrlDown = keyDown;
    else if (scanCode == SCANCODE_RIGHT_CTRL) rightCtrlDown = keyDown;
    pendingScanCode = scanCode;
  }
  oldKbHandler();
}

void register_keyboard_interrupt()
{
  oldKbHandler = getvect(0x09);
  setvect(0x09, key_interrupt);
}

void unregister_keyboard_interrupt()
{
  if (oldKbHandler)
  {
    setvect(0x09, oldKbHandler);
    oldKbHandler = 0;
  }
}

int get_key()
{
  int key = getch();
  if (!key)
    key = 0x100 | getch();
  return key;
}
