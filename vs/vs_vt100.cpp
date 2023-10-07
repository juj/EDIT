// System headers
#include <windows.h>

// Standard library C-style
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>

#include <assert.h>

#include "../screen.h"

int get_text_width()
{
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;
  GetConsoleScreenBufferInfo(hOut, &ScreenBufferInfo);
  return ScreenBufferInfo.srWindow.Right - ScreenBufferInfo.srWindow.Left + 1;
}

int get_text_height()
{
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;
  GetConsoleScreenBufferInfo(hOut, &ScreenBufferInfo);
  return ScreenBufferInfo.srWindow.Bottom - ScreenBufferInfo.srWindow.Top + 1;
}

static struct ScreenChar
{
  char ch;
  unsigned char color;
} *screen_buffer = 0;
static int screen_buffer_size = 0;
static int screen_buffer_width = 0;
static int screen_buffer_height = 0;
static unsigned char current_color = 7;

static void init_screen_buffer()
{
  int w = get_text_width();
  int h = get_text_height();
  if (screen_buffer_width != w || screen_buffer_height != h)
  {
    size_t sz = sizeof(ScreenChar)*w*h;
    screen_buffer = (ScreenChar*)realloc(screen_buffer, sz);
    screen_buffer_width = w;
    screen_buffer_height = h;
    screen_buffer_size = w*h;
    memset(screen_buffer, 0, sz);
  }
}

static int vt_fore_color(unsigned char dosColor)
{
  switch(dosColor&0xF)
  {
  case 0/*black*/: return 30;
  case 1/*blue*/: return 34;
  case 2/*green*/: return 32;
  case 3/*aqua*/: return 36;
  case 4/*red*/: return 31;
  case 5/*purple*/: return 35;
  case 6/*brown*/: return 33;
  case 8/*gray*/: return 90;
  case 9/*lightblue*/: return 94;
  case 10/*lightgreen*/: return 92;
  case 11/*lightaqua*/: return 96;
  case 12/*lightred*/: return 91;
  case 13/*lightpurple*/: return 95;
  case 14/*lightyellow*/: return 93;
  case 15/*lightwhite*/: return 97;
  default: assert(false);
  case 7/*white*/: return 37;
  }
}

static int vt_back_color(unsigned char dosColor)
{
  switch(dosColor>>4)
  {
  case 1/*blue*/: return 44;
  case 2/*green*/: return 42;
  case 3/*aqua*/: return 46;
  case 4/*red*/: return 41;
  case 5/*purple*/: return 45;
  case 6/*brown*/: return 43;
  case 7/*white*/: return 47;
  case 8/*gray*/: return 100;
  case 9/*lightblue*/: return 104;
  case 10/*lightgreen*/: return 102;
  case 11/*lightaqua*/: return 106;
  case 12/*lightred*/: return 101;
  case 13/*lightpurple*/: return 105;
  case 14/*lightyellow*/: return 103;
  case 15/*lightwhite*/: return 107;
  default: assert(false);
  case 0/*black*/: return 40;
  }
}

void set_cursor_pos(int x, int y)
{
  printf("\x1b[%d;%dH", y, x);
}

void put_screen(int linearPos, char ch, unsigned char color)
{
  init_screen_buffer();
  int x = linearPos % get_text_width();
  int y = linearPos / get_text_width();
  printf("\x1b[%d;%dH\x1b[%d;%dm%c", y, x, vt_fore_color(color), vt_back_color(color), ch);
  screen_buffer[linearPos].ch = ch;
  screen_buffer[linearPos].color = color;
}

void put_screen(int linearPos, char ch)
{
  put_screen(linearPos, ch, current_color);
}

char get_screen(int linearPos)
{
  init_screen_buffer();
  assert(linearPos >= 0);
  assert(linearPos < screen_buffer_size);
  return screen_buffer[linearPos].ch;
}

void put_screen_color(int linearPos, unsigned color)
{
  put_screen(linearPos, get_screen(linearPos), current_color);
}

unsigned char get_screen_color(int linearPos)
{
  init_screen_buffer();
  assert(linearPos >= 0);
  assert(linearPos < screen_buffer_size);
  return screen_buffer[linearPos].color;
}

int init_screen()
{
  // Set output mode to handle virtual terminal sequences
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE)
  {
    return false;
  }

  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode))
  {
    return false;
  }

  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(hOut, dwMode))
  {
    return false;
  }

  SCREEN_WIDTH = get_text_width();
  SCREEN_HEIGHT = get_text_height();

  return true;
}

void deinit_screen()
{
}
