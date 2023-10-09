#if defined(_MSC_VER) || defined(__TURBOC__)
#include <conio.h>
#include <dos.h>
#endif
#include "screen.h"
#include "util.h"
#include "state.h"
#include "audio.h"
#include "memory.h"
#include "iassert.h"
#include "line.h"
#include <string.h>

#if defined(__TURBOC__) && defined(__MSDOS__)
int currentVideoMode = TEXT_MODE_80x25;
#endif

unsigned char SCREEN_WIDTH = 80;
unsigned char SCREEN_HEIGHT = 25;

void clear_screen(unsigned char ch, unsigned char color)
{
  for(int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; ++i)
    put_screen(i, ch, color);
}

void clear_line(int screenPos, unsigned char ch)
{
  for(int i = 0; i < SCREEN_WIDTH; ++i)
    put_screen(screenPos + i, ch, BLUE); //DKGRAY | BGCOLOR(BLUE);//editorTextColor;
}

void set_chars_color(int x, int y, int numChars, unsigned char color)
{
  assert(x >= 0);
  assert(x < SCREEN_WIDTH);
  assert(numChars >= 0);
  assert(x + numChars <= SCREEN_WIDTH);
  assert(y >= 0);
  assert(y < SCREEN_HEIGHT);
  int screenPos = y*SCREEN_WIDTH+x;
  for(int i = 0; i < numChars; ++i)
    put_screen_color(screenPos + i, color);
}

void highlight_line(int yLine, unsigned char bgColor)
{
  assert(yLine >= 0);
  assert(yLine < SCREEN_HEIGHT);
  int screenPos = yLine*SCREEN_WIDTH;
  for(int i = 0; i < SCREEN_WIDTH; ++i)
    put_screen_color(screenPos + i, bgColor | get_screen_color(screenPos+i) & 0x0F);
}

void log_x(const char *str)
{
  int x = (SCREEN_WIDTH - (int)strlen(str)) >> 1;
  int pos = ((SCREEN_HEIGHT-1) * SCREEN_WIDTH + x);
  for(int i = 0; str[i] != 0; ++i, ++pos)
    put_screen(pos, str[i], WHITE | BGCOLOR(MAGENTA));
}

void show_line_with_colors(int screenPos, char huge *text, int textLen, unsigned char *colors, int drawStartX, int drawEndX)
{
  textLen = MIN(textLen - viewportXOffset, drawEndX);
  int x;
  for(x = drawStartX; x < textLen; ++x)
    put_screen(screenPos+x, text[x + viewportXOffset], colors[x + viewportXOffset]);
  for(; x < drawEndX; ++x)
    put_screen(screenPos+x, 0, editorTextColor);
}

#ifdef __C64__
  const unsigned char selectionColor = 0x80; // TODO: #define INVERT 0x80
#else
  const unsigned char selectionColor = BLUE | BGCOLOR(LTGRAY);
#endif

void show_line_with_colors_with_sel(int screenPos, char huge *text, int textLen, unsigned char *colors, int selStart, int selEnd, int drawStartX, int drawEndX)
{
  selStart -= viewportXOffset;
  selEnd -= viewportXOffset;
  textLen = MIN(textLen - viewportXOffset, drawEndX);
  int x;
  for(x = drawStartX; x < textLen; ++x)
  {
    put_screen(screenPos + x, text[x + viewportXOffset], x < selStart || x >= selEnd ? colors[x + viewportXOffset] : selectionColor);
//    screenPtr[x<<1] = ;
//    screenPtr[(x<<1)+1] = ;
  }
  for(; x < drawEndX; ++x)
  {
    put_screen(screenPos + x, 0, editorTextColor);
  }
}

void show_line(int screenPos, char huge *text, int textLen)
{
  textLen = MIN(textLen - viewportXOffset, SCREEN_WIDTH);
  int x;
  for(x = 0; x < textLen; ++x)
  {
    put_screen(screenPos + x, text[x + viewportXOffset], editorTextColor);
  }
  for(; x < SCREEN_WIDTH; ++x)
  {
    put_screen(screenPos + x, 0, editorTextColor);
  }
}

void show_line_with_sel(int screenPos, char huge *text, int textLen, int selStart, int selEnd, int drawStartX, int drawEndX)
{
  selStart -= viewportXOffset;
  selEnd -= viewportXOffset;
  textLen = MIN(textLen - viewportXOffset, SCREEN_WIDTH);
  int x;
  for(x = drawStartX; x < textLen; ++x)
  {
    put_screen(screenPos + x, text[x + viewportXOffset], x < selStart || x >= selEnd ? editorTextColor : selectionColor);
  }
  for(; x < drawEndX; ++x)
  {
    put_screen(screenPos + x, 0, editorTextColor);
  }
}

void get_string_on_screen(int x, int y, int maxLength, char *dst)
{
  int screenPos = y*SCREEN_WIDTH+x;
//  char far *screenPtr = &screen[()<<1];
  int i = 0;
  for(; i < maxLength && is_whitespace(get_screen(screenPos + i)); ++i)
    ;
  int len = 0;
  for(; i < maxLength && get_screen(screenPos + i) >= 32; ++i)
    dst[len++] = get_screen(screenPos + i);;
  dst[len] = 0;
}

void put_string_on_screen(const char *str, int x, int y)
{
  int pos = y * SCREEN_WIDTH + x;
  for(int i = 0; str[i] != 0; ++i, ++pos)
    put_screen(pos, str[i]);
}

void put_string_on_screen(const char *str, int x, int y, unsigned char color)
{
  int pos = y * SCREEN_WIDTH + x;
  for(int i = 0; str[i] != 0; ++i, ++pos)
    put_screen(pos, str[i], color);
}

void log_error(const char *errorText)
{
  put_string_on_screen(errorText, 3, SCREEN_HEIGHT-1, YELLOW | BGCOLOR(RED));
  ERROR_BEEP();
}

#if defined(__TURBOC__) && defined(__MSDOS__)

char far *screen = (char far *)MK_FP(0xB800, 0);

void put_screen(int linearPos, char ch)
{
  screen[linearPos<<1] = ch;
}

void put_screen_color(int linearPos, unsigned char color)
{
  screen[(linearPos<<1)+1] = color;
}

void put_screen(int linearPos, char ch, unsigned char color)
{
  linearPos <<= 1;
  screen[linearPos] = ch;
  screen[linearPos+1] = color;
}

char get_screen(int linearPos)
{
  return screen[linearPos<<1];
}

unsigned char get_screen_color(int linearPos)
{
  return screen[(linearPos<<1)+1];
}
#endif
