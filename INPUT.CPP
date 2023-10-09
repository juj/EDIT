#include "input.h"
#if defined(_MSC_VER) || defined(__TURBOC__)
#include <conio.h>
#include <dos.h>
#endif

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include <string.h>
#include "screen.h"
#include "cursor.h"

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
#define kbhit _kbhit
#endif

#ifndef __C64__

bool is_shift_key_currently_down()
{
#if defined(__TURBOC__) && defined(__MSDOS__)
  return leftShiftDown || rightShiftDown;
#elif defined(_MSC_VER)
  return GetAsyncKeyState(VK_SHIFT) || GetAsyncKeyState(VK_LSHIFT) || GetAsyncKeyState(VK_RSHIFT);
#else
  return 0; // XXX TODO
#endif
}

bool is_ctrl_key_currently_down()
{
#if defined(__TURBOC__) && defined(__MSDOS__)
  return leftCtrlDown || rightCtrlDown;
#elif defined(_MSC_VER)
  return GetAsyncKeyState(VK_CONTROL) || GetAsyncKeyState(VK_LCONTROL) || GetAsyncKeyState(VK_RCONTROL);
#else
  return 0; // XXX TODO
#endif
}

bool is_alt_key_currently_down()
{
#if defined(__TURBOC__) && defined(__MSDOS__)
  return leftAltDown || rightAltDown;
#elif defined(_MSC_VER)
  return 0; // XXX TODO
//  return GetAsyncKeyState(VK_CONTROL) || GetAsyncKeyState(VK_LCONTROL) || GetAsyncKeyState(VK_RCONTROL);
#else
  return 0; // XXX TODO
#endif
}
#endif

int is_navigation_action_key(int key)
{
  switch((key & ~KEY_SHIFT_MODIFIER))
  {
  case KEY_LEFT:
  case KEY_RIGHT:
  case KEY_UP:
  case KEY_DOWN:
#ifndef __C64__ // TODO
  case KEY_PGUP:
#endif
  case KEY_PGDN:
  case KEY_HOME:
  case KEY_END:
#ifndef __C64__ // TODO
  case KEY_CTRL_LEFT:
  case KEY_CTRL_RIGHT:
  case KEY_CTRL_UP:
  case KEY_CTRL_DOWN:
  case KEY_CTRL_HOME:
  case KEY_CTRL_END:
#endif
    return 1;
  default:
    return 0;
  }
}

int is_shift_aware_key(int key)
{
  switch(key)
  {
  case KEY_INSERT:
  case KEY_DELETE:
  case KEY_LEFT:
  case KEY_RIGHT:
  case KEY_UP:
  case KEY_DOWN:
#ifndef __C64__ // TODO
  case KEY_PGUP:
#endif
#ifdef __C64__
  case KEY_F2:
#endif
  case KEY_PGDN:
  case KEY_HOME:
  case KEY_END:
    return 1;
  default:
    return 0;
  }
}

#if defined(_MSC_VER) || defined(__TURBOC__)
bool is_get_key_pending()
{
  return _kbhit();
}

int get_key()
{
  int key = _getch();
  if (!key || key == 0xE0)
    key = 0x100 | _getch();
  if (is_shift_key_currently_down() && is_shift_aware_key(key)) key |= KEY_SHIFT_MODIFIER;
  return key;
}
#endif

bool is_shift_key_held(int key)
{
  return key & KEY_SHIFT_MODIFIER;
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

int ask_string_prompt(const char *prompt, char *dst, int maxLen)
{
  put_string_on_screen(prompt, 0, SCREEN_HEIGHT-1, WHITE);
  int prompt_len = (int)strlen(prompt);
  int x = (int)strlen(dst);
  put_string_on_screen(dst, prompt_len, SCREEN_HEIGHT-1, LTGRAY);
  const int pos = (SCREEN_HEIGHT-1)*SCREEN_WIDTH+prompt_len;
  int result = 0;
  while(result == 0)
  {
    set_cursor_pos(prompt_len+x, SCREEN_HEIGHT-1);
    int key = get_key();
    switch(key)
    {
#ifndef __C64__
    case KEY_ESC: result = -1; break;
#endif
    case KEY_ENTER: if (x > 0) result = 1; break;
    case KEY_BACKSPACE:
      if (x > 0)
      {
        dst[--x] = 0;
        put_screen(pos+x, ' ', LTGRAY);
      }
      break;
    default:
      if (x < maxLen && key >= 0x20 && key < 127)
      {
        if (maxLen > 1 || key == 'y' || key == 'Y' || key == 'n' || key == 'N')
        {
          put_screen(pos+x, key, LTGRAY);
          dst[x++] = key;
        }
      }
      break;
    }
  }
  dst[x] = 0;
  if (result < 0) return -1;
  return x;
}

int ask_yes_no_prompt(const char *prompt)
{
  char dst[2] = {0};
  int ret = ask_string_prompt(prompt, dst, 1);
  return ret > 0 && (dst[0] == 'y' || dst[0] == 'Y');
}

#if defined(_MSC_VER)
// For Clang
#pragma comment(lib, "User32.lib")
#endif
