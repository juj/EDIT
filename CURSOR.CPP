#include "cursor.h"
#include "state.h"

void print_insert_status_text(bool print_on_clear);

static bool insertMode = false;

void toggle_insert()
{
#ifdef SUPPORT_INSERT_MODE
  insertMode = !insertMode;
  if (insertMode) set_cursor_style(CURSOR_INSERT);
  else set_cursor_style(CURSOR_NORMAL);
  print_insert_status_text(true);
#endif
}

void set_insert_mode(bool new_insert_mode)
{
#ifdef SUPPORT_INSERT_MODE
  insertMode = new_insert_mode;
#endif
}

bool insert_mode()
{
#ifdef SUPPORT_INSERT_MODE
  return insertMode;
#else
  return false;
#endif
}

#if defined(__TURBOC__) && defined(__MSDOS__)

void set_cursor_style(int style)
{
#if defined(__TURBOC__) && defined(__MSDOS__)
  switch(style)
  {
  case CURSOR_NONE: _setcursortype(_NOCURSOR); break;
  case CURSOR_INSERT: _setcursortype(_SOLIDCURSOR); break;
  default: _setcursortype(_NORMALCURSOR); break;
  }
#else
  // todo
#endif
}

void set_vga_cursor_position(int x, int y)
{
  //  disable();
  union REGS regs = {0};
  regs.h.ah = 0x2;
  regs.h.bh = 0;
  regs.h.dh = y;
  regs.h.dl = x;
  int86(VIDEO_INTERRUPT, &regs, &regs);
  //  enable();
}

void set_cursor_pos(uint8_t x, uint8_t y)
{
  gotoxy(x+1, y+1);
  set_vga_cursor_position(cursorX, cursorY);
}

#endif
