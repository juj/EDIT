#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdint.h>

struct File;

void init_network();
void deinit_network();

// N.b. this is super unsafe, the file is broadcasted to network
int save_file_to_network(File *file);

// Internal:

#define SERVER_PORT 4009
#define CLIENT_PORT 4008

#ifdef BUILD_SERVER
#define RECVFROM_PORT SERVER_PORT
#define SENDTO_PORT CLIENT_PORT
#else
#define RECVFROM_PORT CLIENT_PORT
#define SENDTO_PORT SERVER_PORT
#endif

enum MessageType : uint8_t
{
  MessageSaveFile,
  MessageLoadFile
};

struct SaveFileMsg
{
  char messageId;
  char filename[16];
  uint16_t dataSize;
#pragma warning(disable : 4200)
  char data[];
};

#endif
