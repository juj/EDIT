#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdint.h>
#include "types.h"

struct File;
struct Line;

void init_network();
void deinit_network();

#define EDIT_TYPE_INSERT_LINE 0
#define EDIT_TYPE_REPLACE_LINE 1
#define EDIT_TYPE_DELETE_LINE 2
void send_line_to_network_server(uint8_t editType, File *f, Line *l, line_num lineno);

// N.b. this is super unsafe, the file is broadcasted to network
int save_file_to_network(File *file);

void receive_data_from_network();

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

#pragma pack(push, 1)

#define MAGIC_UPDATE_LINE_MSG 0x56369124
struct UpdateLineMsg
{
  uint32_t magic; // == MAGIC_UPDATE_LINE_MSG
  uint16_t line_no;
  uint8_t edit_type; // if 0, replace existing line
  uint8_t filename_len;
  uint8_t line_len;
  //char filename[filename_len];
  //char line[line_len];
};

#define MAGIC_BUILD_ERROR_MSG 0x31268143
struct BuildErrorMsg
{
  uint32_t magic; // == MAGIC_BUILD_ERROR_MSG
  uint16_t line_no;
  uint8_t column;
  uint8_t success;
  uint8_t msg_len; // If 0, build was ok
  //char msg[msg_len];
};

#pragma pack(pop)

#endif
