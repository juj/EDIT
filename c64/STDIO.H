#pragma once

// Before including this file, call
// #define _STDIO_DEBUG_LEVEL 2 to add verbose debugging support, or
// #define _STDIO_DEBUG_LEVEL 1 to add more lightweight debugging support.
// #define NDEBUG to disable assert() macro.
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include_next <stdio.h>

#include "c64_kernal.h"

#ifndef _STDIO_DEBUG_LEVEL
#define _STDIO_DEBUG_LEVEL 0
#endif

struct FILE
{
  uint8_t nextChar; // for read streams, the next character to come out from the file.
  uint8_t handle;
};

#define MAX_FILENAME_LEN 16

static bool string_contains_char(const char *str, char ch)
{
  assert(str);
  while(*str) if (*str++ == ch) return true;
  return false;
}

static void print_dos_error(uint8_t error, const char *operation)
{
#if _STDIO_DEBUG_LEVEL >= 1
  if (error < 20) return;
  _CLRCHN();
#endif

#if _STDIO_DEBUG_LEVEL >= 2
  const char *err;
  switch(error)
  {
    // From https://archive.org/details/Commodore_1541_Disk_Drive_Users_Guide_1982-09_Commodore/page/n59/mode/2up
    case 20: err = "BLOCK HEADER NOT FOUND"; break; // The disk controUer is unable to locate the header of the requested data block. Caused by an illegal block number, or the header has been destroyed.
    case 21: err = "SYNC CHARACTER NOT FOUND"; break; // The disk controller is unable to detect a sync mark on the desired track. Caused by misalignment of the read/ writer head, no diskette is present, or unformatted or improperly sealed diskette. Can also indicate a hardware failure.
    case 22: err = "DATA BLOCK NOT PRESENT"; break; // The disk controller has been requested to read or verify a data block that was not properly written. This error message occurs in conjunction with the BLOCK commands and indicates an illegal track and/or block request.
    case 23: err = "CHECKSUM ERROR"; break; // This error message indicates that there is an error in one or more of the data bytes. The data has been read into the DOS memory, but the checksum over the data is in error. This message may also indicate grounding problems.
    case 24: err = "BYTE DECODING ERROR"; break; // The data or header as been read into the DOS memory, but a hardware error has been created due to an invalid bit pattern in the data byte. This message may also indicate grounding problems.
    case 25: err = "WRITE-VERIFY ERROR"; break; // This message is generated if the controller detects a mismatch between the written data and the data in the DOS memory.
    case 26: err = "WRITE PROTECT ON"; break; // This message is generated when the controller has been requested to write a data block while the write protect switch is depressed. Typically, this is caused by using a diskette with a write a protect tab over the notch.
    case 27: err = "HEADER CHECKSUM ERROR"; break; // The controller has detected an error in the header of the requested data block. The block has not been read into the DOS memory. This message may also indicate grounding problems.
    case 28: err = "DATA EXTENDS INTO NEXT BLOCK"; break; // The controller attempts to detect the sync mark of the next header after writing a data block. If the sync mark does not appear within a pre-determined time, the error message is generated. The error is caused by a bad diskette format (the data extends into the next block), or by hardware failure.
    case 29: err = "DISK ID MISMATCH"; break; // This message is generated when the controller has been requested to access a diskette which has not been initialized. The message can also occur if a diskette has a bad header.
    case 30: err = "GENERAL SYNTAX ERROR"; break; // The DOS cannot interpret the command sent to the command channel. Typically, this is caused by an illegal number of file names, or patterns are illegally used. For example, two file names may appear on the left side of the COPY command.
    case 31: err = "INVALID COMMAND"; break; // The DOS does not recognize the command. The command must start in the first position,
    case 32: err = "LONG LINE"; break; // The command sent is longer than 58 characters.
    case 33: err = "INVALID FILENAME"; break; // Pattern matching is invalidly used in the OPEN or SAVE command.
    case 34: err = "NO FILE GIVEN"; break; // The file name was left out of a command or the DOS does not recognize it as such. Typically, a colon (:) has been left out of the command.
    case 39: err = "COMMAND FILE NOT FOUND"; break; // This error may result if the command sent to command channel (secondary address 15) is unrecognized by the DOS.
    case 50: err = "RECORD NOT PRESENT"; break; // Result of disk reading past the last record through 1NPUT#, or GET# commands. This message will also occur after positioning to a record beyond end of file in a relative file. If the intent is to expand the file by adding the new record (with a PRINTS command), the error message may be ignored. INPUT or GET should not be attempted after this error is detected without first repositioning.
    case 51: err = "OVERFLOW"; break; // PRINTS statement exceeds record boundary. Information is cut off. Since the carriage return is sent as a record terminator is counted in the record size. This message will occur if the total characters in the record (including the final carriage return) exceeds the defined size.
    case 52: err = "FILE TOO LARGE"; break; // Record position within a relative file indicates that disk overflow will result.
    case 60: err = "FILE OPEN FOR WRITE"; break; // This message is generated when a write file that has not been closed is being opened for reading.
    case 61: err = "FILE NOT OPEN"; break; // This message is generated when a file is being accessed that has not been opened in the DOS. Sometimes, in this case, a message is not generated; the request is simply ignored.
    case 62: err = "FILE NOT FOUND"; break; // The requested file does not exist on the indicated drive.
    case 63: err = "FILE EXISTS"; break; // The file name of the file being created already exists on the diskette.
    case 64: err = "FILE TYPE MISMATCH"; break; // The file type does not match the file type in the directory entry for the requested file.
    case 65: err = "NO BLOCK"; break; // This message occurs in conjunction with the B-A command. It indicates that the block to be allocated has been previously allocated. The parameters indicate the track and sector available with the next highest number. If the parameters are zero (0), then all blocks higher in number arc in use.
    case 66: err = "ILLEGAL TRACK OR SECTOR"; break; // The DOS has attempted to access a irack or block which does not e>dsi in die format being used. This may indicate a problem reading the pointer to the next block.
    case 67: err = "ILLEGAL SYSTEM TRACK OR SECTOR"; break; // This special error message indicates an illegal system track or block.
    case 70: err = "NO CHANNELS AVAILABLE"; break; // The requested channel is not available, or all channels are in use. A maximum of five sequential files may be opened at one time to the DOS, Direct access channels may have six opened files.
    case 71: err = "DIRECTORY ERROR"; break; // The BAM docs not match the internal count. There is a problem in the BAM allocation or the BAM has been overwritten in DOS memory. To correct this problem, reinitialize the diskette to restore the BAM in memory. Some active files may be terminated by the corrective action. NOTE: BAM = Block Availability Map
    case 72: err = "DISK OR DIRECTORY FULL"; break; // Either the blocks on the diskette are used or the directory is at its entry limit. DISK FULL is sent when two blocks are available on the 1541 to allow the current file to be closed.
    case 73: err = "POWER UP MESSAGE; break; OR WRITE ATTEMPT WITH DOS MISMATCH"; break; // DOS I and 2 are read compatible but not write compatible. Disks may be interchangeably read with either DOS, but a disk formatted on one version cannot be written upon with the other version because the format is different. This error is displayed whenever an attempt is made to write upon a disk which has been formatted in a non-compatible format. (A utility routine is available to assist in converting from one format to another.) This message may also appear after power up.
    case 74: err = "DRIVE NOT READY"; break; // An attempt has been made to access the 1541 Single Drive Floppy Disk without any diskettes present in either drive.
    default:
      printf("%s: ERROR %u\n", operation, (unsigned int)error);
      return;
  }
  printf("%s: %s\n", operation, err);
#elif _STDIO_DEBUG_LEVEL >= 1
  printf("%s: ERROR %u\n", operation, (unsigned int)error);
#endif
}

FILE *fopen(const char *filename, const char *mode)
{
  static uint8_t fileHandleCounter = 3;

  char fn[MAX_FILENAME_LEN+3+4]; // Add +3 for "@0:" prefix and +4 for ",U,W" suffix.
  // Prepend "@0:" to the filename to have any existing filename be replaced.
  fn[0] = 64; // '@' in PETSCII
  fn[1] = '0';
  fn[2] = ':';
  uint8_t i = 3;
  assert(filename);
  for(uint8_t j = 0; filename[j]; ++j) { fn[i++] = filename[j]; }
  // Append ",S,R" or ",S,W" to filename to tell that the file type is Sequential, and that
  // we are either reading or writing to the file.
  fn[i++] = ',';
  fn[i++] = 'S';
  fn[i++] = ',';
  bool readingFile = string_contains_char(mode, 'r');
  fn[i++] = readingFile ? 'R' : 'W';
  _SETNAM(fn, i);
  _SETLFS(fileHandleCounter, 8, fileHandleCounter); // todo file ID
  uint8_t error = _OPEN();
  print_dos_error(error, "FOPEN");

  error = readingFile ? _CHKIN(fileHandleCounter) : _CHKOUT(fileHandleCounter);
  print_dos_error(error, "FOPEN/CHKINOUT");

  if ((error = _READST()))
  {
    _CLOSE(fileHandleCounter);
    _CLRCHN();
    print_dos_error(error, "FOPEN/READST");
    return 0;
  }

  FILE *f = (FILE*)malloc(sizeof(FILE));
  if (!f)
  {
    _CLRCHN();
#if _STDIO_DEBUG_LEVEL >= 2
    printf("FOPEN: OOM\n");
#endif
    return 0;
  }
  if (readingFile) f->nextChar = _CHRIN(); // read first char in advance
  f->handle = fileHandleCounter++; // TODO multiple handles
  if (fileHandleCounter == 255) fileHandleCounter = 2;

  return f;
}

int fclose(FILE *f)
{
  assert(f); // The behavior is undefined if the value of the pointer stream is used after fclose returns.
  _CLOSE(f->handle);
  _CLRCHN();
  free(f);
  return 0;
}

static void consume_read_whitespace(FILE *f)
{
  uint8_t error;
  while(isspace(f->nextChar))
  {
    if ((error = _READST()))
    {
      print_dos_error(error, "CONSUMEWS");
      return;
    }
    f->nextChar = _CHRIN();
  }
}

static int read_int_from_file(FILE *f)
{
  uint8_t error;
  assert(isdigit(f->nextChar));
  char intstr[6] = { 0, 0, 0, 0, 0, 0 }; // max length 65536
  uint8_t i = 0;
  while(isdigit(f->nextChar))
  {
    intstr[i++] = f->nextChar;
    if ((error = _READST()))
    {
      print_dos_error(error, "READINT");
      break;
    }
    f->nextChar = _CHRIN();
    if (i >= 5) i = 0; // if i gets this large, read will overflow (will read garbage)
  }
  return atoi(intstr);
}

static void read_string_from_file(FILE *f, char *dst)
{
  uint8_t error;
  assert(!isspace(f->nextChar));
  while(!isspace(f->nextChar))
  {
    if ((error = _READST()))
    {
      print_dos_error(error, "READSTR");
      break;
    }
    *dst++ = f->nextChar;
    f->nextChar = _CHRIN();
  }
}

static int fscanf(FILE *f, const char *format, ...)
{
  _CHKIN(f->handle);

  va_list args;
  va_start(args, format);
  // consume any initial whitespace in input
  int8_t num_parsed = 0;
  uint8_t ch, error = 0;
  consume_read_whitespace(f);

  for(; *format; ++format)
  {
    if (isspace(*format))
    {
      consume_read_whitespace(f);
      while(isspace(*format)) ++format;
    }
    else if (*format != '%')
    {
      if (f->nextChar != *format) goto end;
      if ((error = _READST())) { print_dos_error(error, "FSCANF"); goto end; }
      f->nextChar = _CHRIN();
    }
    else
    {
      ++format;
      switch(*format)
      {
      case 'd':
      {
        if (!isdigit(f->nextChar)) goto end;

        int *dst = va_arg(args, int*);
        *dst = read_int_from_file(f);
        ++num_parsed;
      }
      break;
      case 's':
      {
        if (isspace(f->nextChar)) goto end; // todo not 100% sure if this would happen. Maybe "100asdf" being scanned as "%d %s"?
        char *str = va_arg(args, char*);
        read_string_from_file(f, str);
        ++num_parsed;
      }
      break;
      }
    }
  }
end:
  va_end(args);
  _CLRCHN();
  return num_parsed;
}

size_t fread(void *buffer, size_t size, size_t count, FILE *f)
{
  _CHKIN(f->handle);  
  uint8_t error = 0;
  uint8_t *ptr = (uint8_t*)buffer;
  for(size_t i = 0; i < size; ++i)
    for(size_t j = 0; j < count; ++j)
    {
      *ptr++ = f->nextChar;
      if ((error = _READST())) { print_dos_error(error, "FREAD"); break; }
      f->nextChar = _CHRIN();
    }
  _CLRCHN();
  return (uintptr_t)ptr - (uintptr_t)buffer;
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *f)
{
  uint8_t error = _CHKOUT(f->handle);
  print_dos_error(error, "FWRITE/CHKOUT");
  const uint8_t *ptr = (const uint8_t*)buffer;
  for(size_t i = 0; i < size; ++i)
    for(size_t j = 0; j < count; ++j)
    {
      if ((error = _READST())) { print_dos_error(error, "FWRITE"); break; }
      _CHROUT(*ptr++);
    }
  _CLRCHN();
  return (uintptr_t)ptr - (uintptr_t)buffer;
}

static volatile uint8_t * const rrnet_ctl      = (uint8_t *)0xDE01;
static volatile uint8_t * const cs_packet_page = (uint8_t *)0xDE02;
static volatile uint8_t * const cs_packet_data = (uint8_t *)0xDE04;
static volatile uint8_t * const cs_rxtx_data   = (uint8_t *)0xDE08;
static volatile uint8_t * const cs_tx_cmd      = (uint8_t *)0xDE0C;
static volatile uint8_t * const cs_tx_len      = (uint8_t *)0xDE0E;

static void write_u16_lo_hi(volatile uint8_t *dst, uint16_t value)
{
  dst[0] = (uint8_t)value;
  dst[1] = (uint8_t)(value>>8);
}

static uint16_t read_u16_lo_hi(volatile uint8_t *ptr)
{
  uint8_t lo = ptr[0];
  return (((uint16_t)ptr[1]) << 8) | lo;
}

static uint16_t read_u16_hi_lo(volatile uint8_t *ptr)
{
  uint8_t hi = ptr[1];
  return (((uint16_t)hi) << 8) | ptr[0];
}

static void cs_set_page(uint16_t page) { write_u16_lo_hi(cs_packet_page, page); }
static uint16_t cs_read_page_data()    { return read_u16_hi_lo(cs_packet_data); }
static uint16_t rx_data_lo_hi()        { return read_u16_lo_hi(cs_rxtx_data); }
static uint16_t rx_data_hi_lo()        { return read_u16_hi_lo(cs_rxtx_data); }
static uint16_t ntohs(uint16_t x)      { return (x << 8) | (x >> 8); }

static void cs_write_page_data(uint16_t page, uint16_t data)
{
  cs_set_page(page);
  write_u16_lo_hi(cs_packet_data, data);
}

static int rrnet_init()
{
  *rrnet_ctl |= 1; // enable clock port
  // Detect RR-Net
  cs_set_page(0);
  if (cs_read_page_data() != 0x630E) return 0;
  // Configure chip
  cs_write_page_data(0x0114/*pp_self_ctl*/, 0x0055); // reset chip
  cs_write_page_data(0x0104/*pp_rx_ctl*/, 0x0D05); // accept individual and broadcast packets
//  cs_write_page_data(0x0104/*pp_rx_ctl*/, 0x0D85); // accept individual and broadcast packets (promisc)
  cs_write_page_data(0x0158/*pp_ia*/, 0x8000); // Write MAC Address
  cs_write_page_data(0x015A/*pp_ia*/, 0xB310);
  cs_write_page_data(0x015C/*pp_ia*/, 0xB7EB);
  cs_write_page_data(0x0112/*pp_line_ctl*/, 0xD3); // Enable RX and TX
  return 1;
}

struct eth_frame
{
  unsigned char dst_mac[6];
  unsigned char src_mac[6];
  uint16_t etherType; // Big Endian: https://en.wikipedia.org/wiki/EtherType#Values
};

struct ip_frame
{
  uint8_t versionAndHeaderLength;
  uint8_t typeOfService;
  uint16_t packetLength;
  uint16_t identification;
  uint16_t flagsAndFragmentOffset;
  uint8_t timeToLive;
  uint8_t protocol;
  uint16_t headerChecksum;
  uint8_t src_ip[4];
  uint8_t dst_ip[4];
};

struct udp_header
{
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t length;
  uint16_t checksum;
};

#define MAX_C64_FILENAME_LEN 16

struct udp_magic
{
  uint32_t magic_header;
};

#define RECV_FILE_DATAGRAM_MAGIC 0x25291423
struct recv_file_datagram_header
{
//  uint32_t magic_header;
  char filename[20];
  uint16_t file_id;
  uint16_t num_fragments;
} __attribute__((packed));

#define RECV_FILE_DATAGRAM_FRAGMENT_MAGIC 0x53915315
struct recv_file_datagram_fragment_header
{
//  uint32_t magic_header;
  uint16_t file_id;
  uint16_t fragment_index;
} __attribute__((packed));

static void read_struct(void *dst, uint16_t numBytes)
{
  uint16_t *frame = (uint16_t *)dst;
  numBytes = (numBytes+1)>>1;
  while(numBytes--)
    *frame++ = rx_data_lo_hi();
}

static void discard_rx_bytes(unsigned int numBytes)
{
  numBytes = (numBytes+1)>>1;
  while(numBytes-- > 0)
    rx_data_lo_hi();
}

static void debug_dump_rx_bytes(unsigned int numBytes)
{
  numBytes = (numBytes+1)>>1;
  while(numBytes-- > 0)
    printf("%04X", ntohs(rx_data_lo_hi()));
  printf("\n");
}

static void start_tx(uint16_t numBytes)
{
  write_u16_lo_hi(cs_tx_cmd, 0x00C0); // Send Transmit command 00C0h
  write_u16_lo_hi(cs_tx_len, numBytes);
  cs_set_page(0x0138);
  while (!(cs_read_page_data() & 0x0100))
    ; // Wait until we are ready to transmit
}

static void tx_bytes(const void *msg, uint16_t numBytes)
{
  numBytes = (numBytes+1)>>1;
  const uint16_t *m16 = (const uint16_t*)msg;
  while(numBytes--)
    write_u16_lo_hi(cs_rxtx_data, *m16++);
}

static volatile unsigned char * const screen_mem = (unsigned char*)0x0400;
static volatile unsigned char * const screen_frame = (unsigned char *)0xD020;

static unsigned char ascii_to_petscii(unsigned char ch)
{
  if (ch == 0x08) return 0x14; // Backspace character
  if (ch == 0x0A) return 0x0D; // NL line feed, new line
  if (ch == 0x0B) return 0x11; // Vertical tab
  if (ch == 0x0C) return 0x93; // NP form feed, new page
  if (ch == 0x0D) return 0x0A; // Carriage return
  if (ch == 0x11) return 0x0B; // Device Control 1
  if (ch == 0x14) return 0x08; // Device Control 4
  if (ch == 0x60) return 0xC0; // `
  if (ch == 0x93) return 0x0C; // "
  if (ch >= 0x41 && ch <= 0x5A) return ch + 0x80; // A-Z
  if (ch >= 0x61 && ch <= 0x7A) return ch - 0x20; // a-z
  if (ch >= 0x7B && ch <= 0x7F) return ch + 0x60; // {, |, }, ~ and DEL
  if (ch <= 0xBF || ch >= 0xE0) return ch; // Various other characters that map directly through
  return ch - 0x60;
}

static unsigned char petscii_to_screen_code(unsigned char petscii)
{
  if (petscii <= 31) return 32;//petscii + 128;
  if (petscii <= 63) return petscii;
  if (petscii <= 95) return petscii - 64;
  if (petscii <= 127) return petscii - 32;
  if (petscii <= 159) return 32;//petscii + 64;
  if (petscii <= 191) return petscii - 64;
  if (petscii <= 254) return petscii - 128;
  return 0x5E;
}

uint16_t screen_pos = 0;
static void put_screen(char ch)
{
  screen_mem[screen_pos++] = petscii_to_screen_code(ascii_to_petscii(ch));
  if (screen_pos >= 40*25) screen_pos = 0;
  *screen_frame = ch;
}

static void clear_screen()
{
  memset((void*)screen_mem, 0x20, 40*25);
}

static void close_file()
{
  _CLOSE(2);
  _CLRCHN();
}

// return true on success
bool open_file_for_saving(const char *filename)
{
  char fn[MAX_C64_FILENAME_LEN+1+3+4]; // Add +1 for null, +3 for "@0:" prefix and +4 for ",U,W" suffix.
  // Prepend "@0:" to the filename to have any existing filename be replaced.
  fn[0] = 64; // '@' in PETSCII
  fn[1] = '0';
  fn[2] = ':';
  uint8_t i = 3;
  for(uint8_t j = 0; filename[j]; ++j) { fn[i++] = filename[j]; }
  // Append ",S,W" to filename to tell that the file type is Sequential, and that
  // we are Writing to the file.
  fn[i++] = ',';
  fn[i++] = 'S';
  fn[i++] = ',';
  fn[i++] = 'W';
  fn[i] = 0;
  _SETNAM(fn, i);
  _SETLFS(2, 8, 2);
  uint8_t error = _OPEN();
  // 1541 documentation: https://archive.org/details/Commodore_1541_Disk_Drive_Users_Guide_1982-09_Commodore/page/n57/mode/2up
  // "Error message numbers less than 20 should be ignored with the exception of 01 which gives information about the number of
  // "files scratched" (we are not calling the SCRATCH command here)
  // Oddly I am getting an error 199, so ignore too high error numbers as well. (something is very definitely wrong)
  if (error >= 20 && error <= 74)
  {
    printf("ERROR: %d\n", (int)error);
// TODO: how to do this:
//      ; check drive error channel here to test for
//        ; FILE EXISTS error etc.
    return false;
  }
  _CHKOUT(2);  
  error = _READST();

end:
  if (error)
  {
    close_file();
    printf("FAILED %d\n", error);
    return false;
  }
  return true;
}

uint16_t poll_for_udp_file_transfer_fragment(bool first_fragment)
{
  // Poll if there is an Ethernet frame ready for processing
  cs_set_page(0x0124);
  if ((cs_read_page_data() & 0x0D00) == 0) return false;

  rx_data_hi_lo(); // dummy read to flush status word (ignored)
  uint16_t bytes_left = rx_data_hi_lo();

  union
  {
    eth_frame frame;
    ip_frame ip;
    udp_header udp;
    udp_magic magic;
  } net;

#define READ_STRUCT(s) \
  if (bytes_left < sizeof(s)) goto end; \
  bytes_left -= sizeof(s); \
  read_struct(&s, sizeof(s));

  READ_STRUCT(net.frame);
  if (net.frame.etherType != 0x0008/*IPV4*/) goto end;

  READ_STRUCT(net.ip);
  if (net.ip.protocol != 0x11/*UDP*/) goto end;

  READ_STRUCT(net.udp);

  READ_STRUCT(net.magic);
  if ((first_fragment && net.magic.magic_header == RECV_FILE_DATAGRAM_MAGIC && bytes_left == sizeof(recv_file_datagram_header))
    || (!first_fragment && net.magic.magic_header == RECV_FILE_DATAGRAM_FRAGMENT_MAGIC && bytes_left > sizeof(recv_file_datagram_fragment_header)))
    return bytes_left; // this is interesting data, return it for processing

end:
  discard_rx_bytes(bytes_left);
  return 0;
}

char rrnet_recv_filename[MAX_FILENAME_LEN+1];

const char *receive_file()
{
  uint16_t bytes_left = 0;
  while(!bytes_left)
    bytes_left = poll_for_udp_file_transfer_fragment(true);

  recv_file_datagram_header header;
  read_struct(&header, sizeof(header));
  assert(bytes_left == sizeof(header));

  header.filename[MAX_C64_FILENAME_LEN] = 0;
  strcpy(rrnet_recv_filename, header.filename);
  if (!open_file_for_saving(header.filename))
  {
    printf("FAILED TO OPEN FILE \"%s\"\n", header.filename);
    return 0;
  }
  clear_screen();
  uint8_t origScreenFrameColor;
  origScreenFrameColor = *screen_frame;

  uint32_t totalBytesWritten = 0;
  bool error = false;
  uint16_t next_fragment_index = 0;
  while(next_fragment_index != header.num_fragments)
  {
    uint16_t data_left = poll_for_udp_file_transfer_fragment(false);

    recv_file_datagram_fragment_header frag;
    read_struct(&frag, sizeof(frag));
    data_left -= sizeof(frag);

    if (frag.file_id == header.file_id)
    {
      if (frag.fragment_index != next_fragment_index) { error = true; goto end; }

      while(data_left > 0)
      {
        uint16_t data = rx_data_lo_hi();
        --data_left;
        uint8_t n = 1;
        if (data_left)
        {
          ++n;
          --data_left;
        }
        uint8_t ch[2] = { (char)(data), (char)(data >> 8) };

        for(uint8_t i = 0; i < n; ++i)
        {
          put_screen(ch[i]);
          if (_READST()) { error = true; goto end; }
          _CHROUT(ch[i]);
          ++totalBytesWritten;
        }
      }
      ++next_fragment_index;
    }
    else
      discard_rx_bytes(data_left);
  }

end:
  close_file();
  *screen_frame = origScreenFrameColor;
  _PLOT_SET(0, 0);
  clear_screen();
  if (error) printf("\nFAILED TO WRITE FILE!\n");
  else printf("\nWROTE %lu BYTES TO FILE \"%s\"\n", totalBytesWritten, header.filename);
  return error ? 0 : rrnet_recv_filename;
}

const char *receive_network_file()
{
  if (!rrnet_init())
  {
    printf("NETWORK ADAPTER NOT FOUND!\n");
    return 0;
  }
  printf("WAITING FOR FILE...\n");
  return receive_file();
}
