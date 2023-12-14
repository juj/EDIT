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
#include "network.h"

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
  switch(key)
  {
  case ACTION_CURSOR_LEFT:
  case ACTION_CURSOR_RIGHT:
  case ACTION_CURSOR_UP:
  case ACTION_CURSOR_DOWN:
#ifdef ACTION_CURSOR_TO_PREV_WORD
  case ACTION_CURSOR_TO_PREV_WORD:
#endif
#ifdef ACTION_CURSOR_TO_NEXT_WORD
  case ACTION_CURSOR_TO_NEXT_WORD:
#endif
  case ACTION_CURSOR_TO_LINE_START:
  case ACTION_CURSOR_TO_LINE_END:
#ifdef ACTION_CURSOR_TO_TOGGLE_FILE_START_END
  case ACTION_CURSOR_TO_TOGGLE_FILE_START_END:
#endif
#ifdef ACTION_PAGE_ONE_THIRD_UP
  case ACTION_PAGE_ONE_THIRD_UP:
#endif
#ifdef ACTION_PAGE_ONE_THIRD_DOWN
  case ACTION_PAGE_ONE_THIRD_DOWN:
#endif
  case ACTION_PAGE_UP:
  case ACTION_PAGE_DOWN:
    return true;
  default:
    return false;
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
    key = KEY_EXTENDED_MODIFIER | _getch();
//  if (is_shift_key_currently_down() && is_shift_aware_key(key)) key |= KEY_SHIFT_MODIFIER;
  if (is_shift_key_currently_down()) key |= KEY_SHIFT_MODIFIER;
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

int ask_string_prompt(const char *prompt, char *dst, int maxLen, uint8_t flags)
{
  clear_line((uint16_t)(SCREEN_HEIGHT-1)*SCREEN_WIDTH, ' ', LTGRAY); // Clear last line to start with

  put_string_on_screen(prompt, 0, SCREEN_HEIGHT-1, WHITE);
  uint8_t prompt_len = (uint8_t)strlen(prompt);
  uint8_t x = (uint8_t)strlen(dst);
  put_string_on_screen(dst, prompt_len, SCREEN_HEIGHT-1, LTGRAY);
  const uint16_t pos = (uint16_t)(SCREEN_HEIGHT-1)*SCREEN_WIDTH+prompt_len;
  int8_t result = 0;
  while(result == 0)
  {
    set_cursor_pos(prompt_len+x, SCREEN_HEIGHT-1);
    int key = get_key();
    switch(key)
    {
    case ACTION_ESC: if ((flags & ASK_ALLOW_CANCEL)) result = -1; break;
    case ACTION_ENTER: if ((flags & ASK_ALLOW_EMPTY_INPUT) || x > 0) result = 1; break;
    case ACTION_DELETE_LEFT:
      if (x > 0)
      {
        dst[--x] = 0;
        put_screen(pos+x, ' ', LTGRAY);
      }
      break;
    default:
      if (x < maxLen /*&& prompt_len + x + 1 < SCREEN_WIDTH*/ && key >= 0x20 && key < 127)
      {
        if (((flags & ASK_ONLY_YES_NO) && (key == 'y' || key == 'Y' || key == 'n' || key == 'N'))
          || ((flags & ASK_ONLY_NUMBERS) && (key >= '0' && key <= '9'))
          || (!(flags & ASK_ONLY_YES_NO) && !(flags & ASK_ONLY_NUMBERS)))
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
  int ret = ask_string_prompt(prompt, dst, 1, ASK_ONLY_YES_NO | ASK_ALLOW_CANCEL);
  return ret > 0 && (dst[0] == 'y' || dst[0] == 'Y');
}

int get_key_and_poll_network()
{
  for(;;)
  {
    if (is_get_key_pending()) return get_key();
    else receive_data_from_network();
  }
}

#if defined(_MSC_VER)
// For Clang
#pragma comment(lib, "User32.lib")
#endif
