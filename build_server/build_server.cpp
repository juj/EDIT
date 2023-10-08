#include <stdio.h>
#include "../network.h"
#include "../vs/vs_network.h"

#include <string>

char msg[4096];

void handle_save_file(char *msg, size_t numBytes)
{
  if (numBytes < sizeof(SaveFileMsg)) return;
  SaveFileMsg *save = (SaveFileMsg*)msg;
  if (numBytes < sizeof(SaveFileMsg) + save->dataSize) return;

  std::string filename = std::string("C:\\code\\EDIT\\vs\\network_files\\") + save->filename;
  FILE *handle = fopen(filename.c_str(), "w");
  if (handle)
  {
    fwrite(save->data, 1, save->dataSize, handle);
    fclose(handle);
    printf("Saved file %s, %d bytes.\n", filename.c_str(), save->dataSize);
  }
  else
    printf("Failed to save file %s\n", filename.c_str());
}

void handle_message(char *msg, size_t numBytes)
{
  switch(msg[0])
  {
  case MessageSaveFile: handle_save_file(msg, numBytes); break;
  }
}

void listen_for_messages()
{
  printf("Build server listening for messages.\n");
  for(;;)
  {
    size_t numBytes = recv_datagram(msg, sizeof(msg));
    if (numBytes > 0)
      handle_message(msg, numBytes);
  }
}

int main()
{
  init_network();
  listen_for_messages();
  deinit_network();
}
