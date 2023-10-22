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

File *first_in_file_list(File *f)
{
  while(f->prevFile)
    f = f->prevFile;
  return f;
}

File *last_in_file_list(File *f)
{
  while(f->nextFile)
    f = f->nextFile;
  return f;
}

void bring_to_front_in_mru_order(File *file)
{
  if (!currentFile) // todo remove
    return;

  // Find the frontmost file in MRU order
  File *firstFile = first_in_file_list(currentFile);

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
      if (key == ACTION_ALT_TAB_MENU_NEXT_DOCUMENT
#ifdef ACTION_ALT_TAB_MENU_NEXT_DOCUMENT_ALT2
        || key == ACTION_ALT_TAB_MENU_NEXT_DOCUMENT_ALT2
#endif
        || key == ACTION_ALT_TAB_MENU_PREV_DOCUMENT)
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

void switch_to_next_file(bool nextOrPrev)
{
  // currentFile should always be the first in the list
  assert(!currentFile->prevFile);
  File *next;

  if (nextOrPrev)
  {
    next = currentFile->nextFile;
  }
  else
  {
    next = currentFile;
    while(next->nextFile) next = next->nextFile;
  }
  if (next)
  {
    File *pf = next->prevFile;
    if (pf)
    {
      next->prevFile = 0;
      pf->nextFile = 0;
      File *lastFile = last_in_file_list(next);
      File *firstFile = first_in_file_list(pf);
      lastFile->nextFile = firstFile;
      firstFile->prevFile = lastFile;
    }
    open_file_in_viewport(next, /*bringToMruFront:*/0);
    repaint_open_files_list(0);
  }
}
