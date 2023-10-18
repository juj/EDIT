#pragma once

int rrnet_init();
bool poll_for_receive_file();

void start_udp_send(uint16_t numBytes);
void tx_bytes(const void *msg, uint16_t numBytes);
