#include "../network.h"

void init_network()
{
  rrnet_init();
}

void deinit_network()
{
}

void send_line_to_network_server(uint8_t editType, File *f, Line *l, line_num lineno)
{

}

int save_file_to_network(File *file)
{
  return 0;
}

void receive_data_from_network()
{

}
