#include "file.h"
#include "screen.h"
#include "input.h"
#include "cursor.h"
#include "iassert.h"
#include "util.h"
#include "line.h"
#include <string.h>

#define SAVE_PROMPT 

#define TEXT_COLOR WHITE

// if true, user cancelled
bool ask_name_for_file(File *file)
{
  char fn[MAX_FILENAME_LEN+1];
  fn[MAX_FILENAME_LEN] = 0;
  strcpy(fn, file->filename);
  if (fn[0] == '<') fn[0] = 0;
  int ret = ask_string_prompt("SAVE TO FILE: ", fn, MAX_FILENAME_LEN);
  if (ret < 0) return true; // user cancelled
  strcpy(file->filename, fn);
  return false;
}

#if defined(_MSC_VER) || defined(__TURBOC__)
void save_file(File *file)
{
  assert(file);
  if (!strcmp(file->filename, "<unsaved>"))
  {
    NOTIMPL("Save As.. dialog not yet implemented");
    return;
  }
  Line far *line = file->firstLine;

  FILE *handle = fopen(file->filename, "wb");
  if (!handle)
    goto error;

  while(line)
  {
    long numWritten = huge_fwrite(line->text, line->length, handle);
    if (numWritten != line->length)
    {
      fclose(handle);
      goto error;
    }
    if (line->nextLine)
    {
      size_t written = fwrite("\r\n", 1, 2, handle);
      if (written != 2)
        goto error;
      line = line->nextLine;
    }
    else break;

  }
  fclose(handle);
  file->hasUnsavedChanges = 0;
error:
  char str[64];
  sprintf(str, "Unable to save file \"%s\"!\n", file->filename);
  printf(str);
  return;
}
#endif
