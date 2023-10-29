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

void send_new_document_to_network_server(File *f, bool clear_contents);
void send_ask_compiled_document_msg(uint8_t document_id);

void receive_data_from_network();
void wait_to_receive_file(char *dstFilename);

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

#pragma pack(push, 1)

#define MAGIC_ASK_COMPILED_DOCUMENT_MSG 0x69392AA2
struct AskCompiledDocumentMsg // C64 asks the build server to download the compiled .PRG file to local disk
{
  uint32_t magic; // == MAGIC_ASK_COMPILED_DOCUMENT_MSG
  uint8_t document_id; // an ID of the file to get
  uint8_t dummy; // pad to multiple of 16bits
};

#define MAGIC_NEW_DOCUMENT_MSG 0x2395173A
struct NewDocumentMsg // Used when either opening a new document, loading an existing one, or renaming a document when saving
{
  uint32_t magic; // == MAGIC_NEW_DOCUMENT_MSG
  uint8_t clear_contents; // Boolean: if true, all lines of the document will be erased.
  uint8_t document_id; // an ID to assign to the given document
  uint8_t filename_len;
  uint8_t dummy; // pad to multiple of 16bits
  //char filename[filename_len];
};

#define MAGIC_UPDATE_LINE_MSG 0x56369124
struct UpdateLineMsg
{
  uint32_t magic; // == MAGIC_UPDATE_LINE_MSG
  uint16_t line_no;
  uint8_t edit_type; // see EDIT_TYPE_* above
  uint8_t document_id;
  uint8_t line_len;
  uint8_t dummy; // pad to multiple of 16bits
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
  uint8_t dummy;
  //char msg[msg_len];
};

#pragma pack(pop)

#endif
