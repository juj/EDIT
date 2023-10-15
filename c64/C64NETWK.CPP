#include "../network.h"
#include "../file.h"
#include "../line.h"
#include "c64rrnet.h"
#include <string.h>

// #define ENABLE_NETWORKING

void init_network()
{
#ifdef ENABLE_NETWORKING
  rrnet_init();
#endif
}

void deinit_network()
{
}

void send_line_to_network_server(uint8_t editType, File *f, Line *l, line_num lineno)
{
#ifdef ENABLE_NETWORKING
  UpdateLineMsg m;
  m.magic = MAGIC_UPDATE_LINE_MSG;
  m.line_no = lineno;
  m.edit_type = editType;
  m.filename_len = (uint8_t)strlen(f->filename);
  if (m.filename_len&1) ++m.filename_len;
  m.line_len = l ? l->length : 0;
  start_tx(sizeof(UpdateLineMsg) + m.filename_len + l->length);
  tx_bytes(&m, sizeof(m)); // send header
  tx_bytes(f->filename, m.filename_len); // send filename
  tx_bytes(l->text, l->length); // send line data
#endif
}

int save_file_to_network(File *file)
{
  return 0;
}

void receive_data_from_network()
{
  poll_for_receive_file();
}
