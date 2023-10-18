#include "../file.h"
#include "../line.h"
#include "../input.h"
#include "c64_kernal.h"
#include <string.h>
#include <stdint.h>

void save_file(File *file)
{
#if 0
  char fn[MAX_FILENAME_LEN+1+3+4]; // Add +1 for null, +3 for "@0:" prefix and +4 for ",U,W" suffix.
  // Prepend "@0:" to the filename to have any existing filename be replaced.
  fn[0] = 64; // '@' in PETSCII
  fn[1] = '0';
  fn[2] = ':';
  uint8_t i = 3;
  for(uint8_t j = 0; file->filename[j]; ++j) { fn[i++] = file->filename[j]; }
  // Append ",S,W" to filename to tell that the file type is Sequential, and that
  // we are Writing to the file.
  fn[i++] = ',';
  fn[i++] = 'S';
  fn[i++] = ',';
  fn[i++] = 'W';
  fn[i] = 0;
  _SETNAM(fn, i);
  _SETLFS(2, 8, 2);
  uint8_t error = _OPEN();
  // 1541 documentation: https://archive.org/details/Commodore_1541_Disk_Drive_Users_Guide_1982-09_Commodore/page/n57/mode/2up
  // "Error message numbers less than 20 should be ignored with the exception of 01 which gives information about the number of
  // "files scratched" (we are not calling the SCRATCH command here)
  // Oddly I am getting an error 199, so ignore too high error numbers as well. (something is very definitely wrong)
  if (error >= 20 && error <= 74)
  {
    printf("ERROR: %d\n", (int)error);
// TODO: how to do this:
//      ; check drive error channel here to test for
//        ; FILE EXISTS error etc.
    get_key();
    return;
  }
  _CHKOUT(2);

  Line *l = file->firstLine;
  while(l)
  {
    for(uint16_t i = 0; i < (uint16_t)l->length; ++i)
    {
      if ((error = _READST())) goto end;
      _CHROUT(l->text[i]);
    }
    if ((error = _READST())) goto end;
    _CHROUT('\n');
    l = l->nextLine;
  }
end:
  _CLOSE(2);
  _CLRCHN();
#endif

  bool error = false;
  FILE *handle = fopen(file->filename, "w");
  if (!handle) error = true;
  else
  {
    for(Line *l = file->firstLine; l; l = l->nextLine)
    {
      //if (handle->error) { printf("EXP %d\n", (int)handle->error); }
      //if (fputs(l->text, handle) == EOF) { printf("FPUTS %d!\n", (int)handle->error); error = true; break; }
      if (fwrite(l->text, l->length, 1, handle) != l->length) { printf("FWRITE!\n"); error = true; break; }
      if (fputc('\n', handle) == EOF) { printf("FPUTC!\n"); error = true; break; }
    }
    fclose(handle);
  }

  if (error)
  {
    printf("FAILED %d\n", error);
    get_key();
  }
  else
    file->hasUnsavedChanges = 0;
}
