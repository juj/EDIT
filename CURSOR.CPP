#include "cursor.h"
#include "state.h"

void toggle_insert()
{
  insertMode = !insertMode;
#if defined(__TURBOC__) && defined(__MSDOS__)
  _setcursortype(insertMode ? _SOLIDCURSOR : _NORMALCURSOR);
#else
  // xxx todo
#endif
}

#if defined(__TURBOC__) && defined(__MSDOS__)

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

void set_cursor_pos(int x, int y)
{
  gotoxy(x+1, y+1);
  set_vga_cursor_position(cursorX, cursorY);
}
#endif
