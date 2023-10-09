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
void ask_name_for_file(File *file)
{
  char filename[16] = {};
  if (!is_file_unnamed(file))
    strncpy(filename, file->filename, sizeof(filename)-1);
  int len = ask_string_prompt("SAVE TO FILE: ", filename, 15);
  file->filename = (char*)realloc(file->filename, len+1);
  strncpy(file->filename, filename, len);
  file->filename[len] = 0;
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
  return;

error:
  char str[64];
  sprintf(str, "Unable to save file \"%s\"!", file->filename);
  log_error(str);
  return;
}
#endif
