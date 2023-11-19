#pragma once

#include <stdio.h>
#include <stdint.h>

#define MAX_C64_FILENAME_LEN 16

struct recv_file_datagram_header
{
//  uint32_t magic_header;
  char filename[20];
  uint16_t file_id;
  uint16_t num_fragments;
  uint16_t file_start_address;
  uint16_t prg_entry_func_address;
  uint16_t file_size;
  uint8_t dummy0;
  uint8_t filetype;
} __attribute__((packed));

struct NFILE
{
  bool have_next_byte;
  uint8_t next_byte;
  uint16_t bytes_left;
  uint8_t fragment_index;
  bool eof;
  recv_file_datagram_header header;
};

int rrnet_init(bool enable_promiscuous_mode);
void start_udp_send(uint16_t numBytes);
void send_udp_fragment_received_msg(uint16_t fragment_index);

NFILE *nfopen(const char *filename, const char *mode);
int nfgetc(NFILE *f);
//char *nfgets(char *str, int count, NFILE *f);
size_t nfread(void *buffer, size_t size, size_t count, NFILE *f);
//size_t nfwrite(const void *buffer, size_t size, size_t count, NFILE *f);
int nfclose(NFILE *f);
