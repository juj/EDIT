#include <conio.h>
#include "screen.h"
#include "util.h"
#include "state.h"
#include "audio.h"
#include "memory.h"
#include <dos.h>
#include <assert.h>
#include <string.h>

#include <stdio.h> // todo remove

//char far *screen = (char far *)MK_FP(0xB800, 0);
//char far *screen = (char far *)(uintptr_t)0xB8000000;

extern int currentVideoMode = TEXT_MODE_80x25;
unsigned char SCREEN_WIDTH = 80;
unsigned char SCREEN_HEIGHT = 25;

void clear_screen(unsigned char ch, unsigned char color)
{
  for(int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT*2; i+= 2)
  {
    screen[i] = ch;
    screen[i+1] = color;
  }
}

void clear_line(char far *screenPtr, unsigned char ch)
{
  for(int i = 0; i < SCREEN_WIDTH; ++i)
  {
    screenPtr[i<<1] = ch;
    screenPtr[(i<<1) + 1] = BLUE;//DKGRAY | BGCOLOR(BLUE);//editorTextColor;
  }
}

void set_chars_color(int x, int y, int numChars, unsigned char color)
{
  assert(x >= 0);
  assert(x < SCREEN_WIDTH);
  assert(numChars >= 0);
  assert(x + numChars <= SCREEN_WIDTH);
  assert(y >= 0);
  assert(y < SCREEN_HEIGHT);
  char far *screenPtr = &screen[(y*SCREEN_WIDTH+x)<<1];
  for(int i = 0; i < numChars; ++i)
  {
    screenPtr[(i<<1) + 1] = color;
  }
}

void highlight_line(int yLine, unsigned char bgColor)
{
  assert(yLine >= 0);
  assert(yLine < SCREEN_HEIGHT);
  char far *screenPtr = &screen[yLine*(SCREEN_WIDTH*2)];
  for(int i = 0; i < SCREEN_WIDTH; ++i)
  {
    screenPtr[(i<<1) + 1] = bgColor | (screenPtr[(i<<1) + 1] & 0x0F);
  }
}

void log_x(const char *str)
{
  int x = (SCREEN_WIDTH - (int)strlen(str)) >> 1;
  int pos = ((SCREEN_HEIGHT-1) * SCREEN_WIDTH + x) << 1;
  for(int i = 0; str[i] != 0; ++i, pos += 2)
  {
    screen[pos] = str[i];
    screen[pos+1] = WHITE | BGCOLOR(MAGENTA);
  }
}

void show_line_with_colors(char far *screenPtr, char huge *text, int textLen, unsigned char *colors)
{
  textLen = MIN(textLen - viewportXOffset, SCREEN_WIDTH);
  int x;
  for(x = 0; x < textLen; ++x)
  {
    screenPtr[x<<1] = text[x + viewportXOffset];
    screenPtr[(x<<1)+1] = colors[x + viewportXOffset];
  }
  for(; x < SCREEN_WIDTH; ++x)
  {
    screenPtr[x<<1] = 0;
    screenPtr[(x<<1)+1] = editorTextColor;
  }
}

void show_line_with_colors_with_sel(char far *screenPtr, char huge *text, int textLen, unsigned char *colors, int selStart, int selEnd)
{
  selStart -= viewportXOffset;
  selEnd -= viewportXOffset;
  const unsigned char selectionColor = BLUE | BGCOLOR(LTGRAY);
  textLen = MIN(textLen - viewportXOffset, SCREEN_WIDTH);
  int x;
  for(x = 0; x < textLen; ++x)
  {
    screenPtr[x<<1] = text[x + viewportXOffset];
    screenPtr[(x<<1)+1] = x < selStart || x >= selEnd ? colors[x + viewportXOffset] : selectionColor;
  }
  for(; x < SCREEN_WIDTH; ++x)
  {
    screenPtr[x<<1] = 0;
    screenPtr[(x<<1)+1] = editorTextColor;
  }
}

void show_line(char far *screenPtr, char huge *text, int textLen)
{
  textLen = MIN(textLen - viewportXOffset, SCREEN_WIDTH);
  int x;
  for(x = 0; x < textLen; ++x)
  {
    screenPtr[x<<1] = text[x + viewportXOffset];
    screenPtr[(x<<1)+1] = editorTextColor;
  }
  for(; x < SCREEN_WIDTH; ++x)
  {
    screenPtr[x<<1] = 0;
    screenPtr[(x<<1)+1] = editorTextColor;
  }
}

void show_line_with_sel(char far *screenPtr, char huge *text, int textLen, int selStart, int selEnd)
{
  selStart -= viewportXOffset;
  selEnd -= viewportXOffset;
  const unsigned char selectionColor = BLUE | BGCOLOR(LTGRAY);
  textLen = MIN(textLen - viewportXOffset, SCREEN_WIDTH);
  int x;
  for(x = 0; x < textLen; ++x)
  {
    screenPtr[x<<1] = text[x + viewportXOffset];
    screenPtr[(x<<1)+1] = x < selStart || x >= selEnd ? editorTextColor : selectionColor;
  }
  for(; x < SCREEN_WIDTH; ++x)
  {
    screenPtr[x<<1] = 0;
    screenPtr[(x<<1)+1] = editorTextColor;
  }
}

void get_string_on_screen(int x, int y, int maxLength, char *dst)
{
  char far *screenPtr = &screen[(y*SCREEN_WIDTH+x)<<1];
  int i = 0;
  for(; i < maxLength && screenPtr[i << 1] <= 32; ++i)
    ;
  int len = 0;
  for(; i < maxLength && screenPtr[i << 1] >= 32; ++i)
    dst[len++] = screenPtr[i << 1];
  dst[len] = 0;
}

void put_string_on_screen(const char *str, int x, int y)
{
  int pos = (y * SCREEN_WIDTH + x) << 1;
  for(int i = 0; str[i] != 0; ++i, pos += 2)
    screen[pos] = str[i];
}

void put_string_on_screen(const char *str, int x, int y, unsigned char color)
{
  int pos = (y * SCREEN_WIDTH + x) << 1;
  for(int i = 0; str[i] != 0; ++i, pos += 2)
  {
    screen[pos] = str[i];
    screen[pos+1] = color;
  }
}

void log_error(const char *errorText)
{
  put_string_on_screen(errorText, 3, SCREEN_HEIGHT-1, YELLOW | BGCOLOR(RED));
  ERROR_BEEP();
}
