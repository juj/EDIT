#pragma once

void init_network();
void deinit_network();

void send_datagram(const void *src, size_t numBytes);
size_t recv_datagram(void *dst, size_t maxBytes);
