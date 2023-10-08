#include "../network.h"
#include "../vs/vs_network.h"
#include "../file.h"
#include "../line.h"
#include <string.h>

int save_file_to_network(File *file)
{
  long file_size = calculate_file_byte_size(file);
  long msg_size = sizeof(SaveFileMsg) + file_size;
  SaveFileMsg *msg = (SaveFileMsg*)malloc(msg_size);
  msg->messageId = MessageSaveFile;
  strncpy(msg->filename, file->filename, sizeof(msg->filename));
  msg->dataSize = file_size;
  char *ptr = (char*)msg->data;

  Line *l = file->firstLine;
  while(l)
  {
    memcpy(ptr, l->text, l->length);
    ptr += l->length;
    *ptr++ = '\n';
    l = l->nextLine;
  }
  send_datagram(msg, msg_size);
  free(msg);
  return 1;
}
