#include "../network.h"
#include "../file.h"
#include "../line.h"
#include "c64rrnet.h"
#include <string.h>

#define ENABLE_NETWORKING

bool network_initialized = false;
void init_network()
{
#ifdef ENABLE_NETWORKING
  network_initialized = rrnet_init();
#endif
}

void deinit_network()
{
}

void send_new_document_to_network_server(File *f, bool clear_contents)
{
#ifdef ENABLE_NETWORKING
  if (!network_initialized) return;

  NewDocumentMsg m;
  m.magic = MAGIC_NEW_DOCUMENT_MSG;
  m.clear_contents = clear_contents;
  m.document_id = f->document_id;
  m.filename_len = (uint8_t)strlen(f->filename);
  uint8_t orig_filename_len = m.filename_len;
  if (m.filename_len&1) ++m.filename_len;
  start_udp_send(sizeof(m) + m.filename_len);
  tx_bytes(&m, sizeof(m)); // send header
  tx_bytes(f->filename, orig_filename_len); // send filename
#endif  
}

void send_line_to_network_server(uint8_t editType, File *f, Line *l, line_num lineno)
{
#ifdef ENABLE_NETWORKING
  if (!network_initialized) return;

  UpdateLineMsg m;
  m.magic = MAGIC_UPDATE_LINE_MSG;
  m.line_no = lineno;
  m.edit_type = editType;
  m.document_id = f->document_id;
  m.line_len = l ? l->length : 0;
  uint8_t orig_line_len = m.line_len;
  if (m.line_len&1) ++m.line_len;
  start_udp_send(sizeof(m) + m.line_len);
  tx_bytes(&m, sizeof(m)); // send header
  tx_bytes(l->text, orig_line_len); // send line data
#endif
}

void receive_data_from_network()
{
#ifdef ENABLE_NETWORKING
  poll_for_receive_file();
#endif
}
