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
    for(Line *l = file->firstLine; l; l = l->nextLine)
    {
      if (fwrite8(l->text, l->length, handle) != l->length)
      {
        error = true;
        break;
      }
      if (fputc('\n', handle) == EOF)
      {
        error = true;
        break;
      }
    }
    fclose(handle);
  }

  if (error)
  {
    write_status_text("FILE WRITE FAILED", LTRED);
    get_key();
  }
  else
    file->hasUnsavedChanges = 0;
}
