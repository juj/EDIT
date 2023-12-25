#include "../file.h"
#include "../line.h"
#include "../input.h"
#include "../screen.h"
#include "c64_kernal.h"
#include <string.h>
#include <stdint.h>

void save_file(File *file, char *device_and_mode)
{
  bool error = false;
  file->deviceNum = device_and_mode[0] ? atoi(device_and_mode) : 8;
  strcat(device_and_mode, "w");
  FILE *handle = fopen(file->filename, device_and_mode);
  if (!handle) error = true;
  else
  {
  _DEBUG_SCREEN_CHAR('0');
    for(Line *l = file->firstLine; l; l = l->nextLine)
    {
  _DEBUG_SCREEN_CHAR('1');
      if (fwrite8(l->text, l->length, handle) != l->length)
      {
  _DEBUG_SCREEN_CHAR('2');
        error = true;
  _DEBUG_SCREEN_CHAR('3');
        break;
      }
      if (fputc('\n', handle) == EOF)
      {
  _DEBUG_SCREEN_CHAR('4');
        error = true;
  _DEBUG_SCREEN_CHAR('5');
        break;
      }
    }
  _DEBUG_SCREEN_CHAR('6');
    fclose(handle);
  _DEBUG_SCREEN_CHAR('7');
  }
  _DEBUG_SCREEN_CHAR('8');

  if (error)
  {
    write_status_text("FILE WRITE FAILED", LTRED);
    get_key();
  }
  else
    file->hasUnsavedChanges = 0;
}
