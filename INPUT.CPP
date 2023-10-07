#include "input.h"
#include <conio.h>
#include <dos.h>

#ifdef _MSC_VER
#include <Windows.h>
#endif

#if defined(__TURBOC__) && defined(__MSDOS__)

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
#endif

#if defined(__TURBOC__) && defined(__MSDOS__)
// 'getch': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _getch.
#define getch _getch
#endif

int get_key()
{
  int key = _getch();
  if (!key || key == 0xE0)
    key = 0x100 | _getch();
  return key;
}

void init_keyboard()
{
#if defined(__TURBOC__) && defined(__MSDOS__)
  register_keyboard_interrupt();
#endif
}

void deinit_keyboard()
{
#if defined(__TURBOC__) && defined(__MSDOS__)
  unregister_keyboard_interrupt();
#endif
}

bool is_shift_key_held()
{
#if defined(__TURBOC__) && defined(__MSDOS__)
  return leftShiftDown || rightShiftDown;
#elif defined(_MSC_VER)
  return GetAsyncKeyState(VK_SHIFT) || GetAsyncKeyState(VK_LSHIFT) || GetAsyncKeyState(VK_RSHIFT);
#else
  return 0; // XXX TODO
#endif
}
