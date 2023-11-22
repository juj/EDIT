#pragma once

int rrnet_init();
bool process_udp_network_datagrams(char *dstFilename);
void wait_to_receive_file(char *dstFilename);

void start_udp_send(uint16_t numBytes);
void tx_bytes(const char *msg, uint16_t numBytes);
