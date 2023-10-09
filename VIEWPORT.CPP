#include "viewport.h"
#include "state.h"
#include "file.h"
#include "input.h"
#include "audio.h"
#include "iassert.h"
#if defined(_MSC_VER) || defined(__TURBOC__)
#include <conio.h>
#endif

void repaint_open_files_list(File *highlightFilename);
void repaint_screen();

void bring_to_front_in_mru_order(File *file)
{
  if (!currentFile)
    return;

  // Find the frontmost file in MRU order
  File *firstFile = currentFile;
  while(firstFile->prevFile)
    firstFile = firstFile->prevFile;

  if (firstFile == file)
    return;

  // Remove this from its linked list.
  if (file->prevFile) file->prevFile->nextFile = file->nextFile;
  if (file->nextFile) file->nextFile->prevFile = file->prevFile;

  file->nextFile = firstFile;
  file->prevFile = 0;
  firstFile->prevFile = file;
}

void open_file_in_viewport(File *file, unsigned char bringToMruFront)
{
  if (bringToMruFront) bring_to_front_in_mru_order(file);

  if (file == currentFile)
    return; // File is already currently open in viewport, nothing to do.

  currentFile = file;

  // TODO: Remember the viewport for each file
  currentLine = firstDisplayedLine = file->firstLine;
  firstDisplayedLineNumber = 1;
  cursorX = virtualX = cursorY = 0;
  repaint_screen();
}

void run_alt_tab_menu()
{
  File *f = currentFile;
  if (f->nextFile) f = f->nextFile;
  else if (f->prevFile) f = f->prevFile;

  open_file_in_viewport(f, /*bringToMruFront:*/0);
  repaint_open_files_list(f);

  while(is_ctrl_key_currently_down() || is_alt_key_currently_down())
  {
    if (is_get_key_pending())
    {
      int key = get_key();
      if (key == KEY_TAB || key == KEY_ALT_TAB || key == KEY_SHIFT_TAB || key == KEY_CTRL_TAB)
      {
        if (is_shift_key_currently_down())
        {
          if (f->prevFile) f = f->prevFile;
            else while(f->nextFile) f = f->nextFile;
        }
        else
        {
          if (f->nextFile) f = f->nextFile;
          else while(f->prevFile) f = f->prevFile;
        }

        open_file_in_viewport(f, /*bringToMruFront:*/0);
        repaint_open_files_list(f);
      }
    }
  }
  bring_to_front_in_mru_order(f);
}
