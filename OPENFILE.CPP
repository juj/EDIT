#if defined(_MSC_VER) || defined(__TURBOC__)
#include <conio.h>
#include <dos.h>
#endif
#include <stdio.h>
#include "iassert.h"
#include "openfile.h"
#include "input.h"
#include "screen.h"
#include "util.h"
#include "state.h"
#include "audio.h"
#include "file.h"
#include "viewport.h"

void open_file(const char *filename)
{
  File *f = load_file(filename);
  if (f)
    open_file_in_viewport(f, /*bringToMruFront:*/1);
  else
    ERROR_BEEP();
}

#if defined(__TURBOC__) && defined(__MSDOS__)

void run_open_file_menu()
{
  struct find_t ffblk;
  int x = 0;
  int y = 1;
  const int FILE_WIDTH = 13;
  char str[FILE_WIDTH+1] = "          .. ";
  put_string_on_screen(str, 0, 0, LTGRAY);
  for(int done = _dos_findfirst("*.*", _A_NORMAL, &ffblk);
      !done;
      done = _dos_findnext(&ffblk))
  {
    sprintf(str, "%12s ", ffblk.name);
    put_string_on_screen(str, x, y++, LTGRAY);
    if (y >= SCREEN_HEIGHT)
    {
      y = 0;
      x += FILE_WIDTH;
      if (x + FILE_WIDTH > SCREEN_WIDTH)
	      break;
    }
  }
  int selectionX = 0;
  int selectionY = 0;
  set_chars_color(selectionX*FILE_WIDTH, selectionY, FILE_WIDTH-1, WHITE | BGCOLOR(LTGRAY));
  done = 0;
  while(!done)
  {
    int key = get_key();
    if (key == KEY_ESC) break;
    if (key == KEY_ENTER)
    {
      char fname[FILE_WIDTH];
      get_string_on_screen(selectionX*FILE_WIDTH, selectionY, FILE_WIDTH-1, fname);
      open_file(fname);
      break;
    }
    if (key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT)
    {
      set_chars_color(selectionX*FILE_WIDTH, selectionY, FILE_WIDTH-1, LTGRAY);

      if (key == KEY_UP) selectionY = MAX(0, selectionY-1);
      else if (key == KEY_DOWN) selectionY = MIN(SCREEN_HEIGHT-1, selectionY + 1);
      else if (key == KEY_LEFT) selectionX = MAX(0, selectionX - 1);
      else/* if (key == KEY_RIGHT)*/ selectionX = MIN(SCREEN_WIDTH / (FILE_WIDTH+1), selectionX + 1);

      set_chars_color(selectionX*FILE_WIDTH, selectionY, FILE_WIDTH-1, WHITE | BGCOLOR(LTGRAY));
    }
  }
}
#else

void run_open_file_menu()
{
  char filename[16] = {};
  ask_string_prompt("OPEN FILE: ", filename, 15);
  open_file(filename);
}

#endif
