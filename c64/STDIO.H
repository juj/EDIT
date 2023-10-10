#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include_next <stdio.h>

struct FILE
{
  uint8_t nextChar; // for read streams, the next character to come out from the file.
  uint8_t handle;
};

static void _assert(bool condition, const char *str)
{
  if (!condition)
  {
    printf("ASSERT FAILED!\n");
    printf("%s", str);
  }
}

#define assert(x) _assert((x), #x);

static void SETNAM(const char *filename, uint8_t filenameLength)
{
  asm volatile("JSR $FFBD\n" // FFBD = SETNAM
                : // outputs
                : "a"(filenameLength), "x"((unsigned char)(uintptr_t)filename), "y"((unsigned char)((uintptr_t)filename>>8))
                :);
}

static void SETLFS(unsigned char logicalFileNum, unsigned char deviceNum, unsigned char secondaryAddr)
{
  asm volatile("JSR $FFBA\n" // FFBA = SETLFS
                : // outputs
                : "a"(logicalFileNum), "x"(deviceNum), "y"(secondaryAddr) // inputs
                :); // clobbers
}

// https://www.c64-wiki.com/wiki/Zeropage: $0002 is unused so safe to access.
//static volatile unsigned char * const tmp = (unsigned char *)0x0002; // arbitrary safe temporary memory address for getting data out of inline assembly

static uint8_t OPEN()
{
  unsigned char errorCode;
  asm volatile("JSR $FFC0\n" // FFC0 = OPEN
                : "=a" (errorCode) // outputs
                : // inputs
                : "x", "y", "c"); // clobbers
  return errorCode;
}

static void CLOSE(unsigned char logicalFileNum)
{
  asm volatile("JSR $FFC3\n" // FFC3 = CLOSE
                : // outputs
                : "a"(logicalFileNum) // inputs
                : "x", "y"); // clobbers
}

static void CHKIN(unsigned char logicalFileNum)
{
  asm volatile("JSR $FFC6\n" // FFC6 = CHKIN
                : // outputs
                : "x"(logicalFileNum) // inputs
                : "a"); // clobbers
}

static void CHKOUT(unsigned char logicalFileNum)
{
  asm volatile("JSR $FFC9\n" // FFC9 = CHKOUT
                : // outputs
                : "x"(logicalFileNum) // inputs
                : "a"); // clobbers
}

static uint8_t READST()
{
  unsigned char statusByte;
  asm volatile("JSR $FFB7\n" // FFB7 = READST Read I/O status byte
                : "=a" (statusByte) // outputs
                : // inputs
                :); // clobbers
  return statusByte;
}

static uint8_t CHRIN()
{
  unsigned char chrin;
  asm volatile("JSR $FFCF\n" // FFCF = CHRIN
                : "=a"(chrin) // outputs
                : // inputs
                : "y"); // clobbers
  return chrin;
}

static void CHROUT(uint8_t byteToWrite)
{
  asm volatile("JSR $FFD2\n" // FFD2 = CHROUT
                :
                : "a"(byteToWrite) // inputs
                : ); // clobbers
}

static void CLRCHN()
{
  asm volatile("JSR $FFCC\n" // FFCC = CLRCHN - Restore default I/O channels
                : // outputs
                : // inputs
                : "a", "x"); // clobbers
}

#define MAX_FILENAME_LEN 16

static bool string_contains_char(const char *str, char ch)
{
  while(*str) if (*str++ == ch) return true;
  return false;
}

FILE *fopen(const char *filename, const char *mode)
{
  static uint8_t fileHandleCounter = 3;

  char fn[MAX_FILENAME_LEN+1+3+4]; // Add +1 for null, +3 for "@0:" prefix and +4 for ",U,W" suffix.
  // Prepend "@0:" to the filename to have any existing filename be replaced.
  fn[0] = 64; // '@' in PETSCII
  fn[1] = '0';
  fn[2] = ':';
  uint8_t i = 3;
  for(uint8_t j = 0; filename[j]; ++j) { fn[i++] = filename[j]; }
  // Append ",S,R" or ",S,W" to filename to tell that the file type is Sequential, and that
  // we are either reading or writing to the file.
  fn[i++] = ',';
  fn[i++] = 'S';
  fn[i++] = ',';
  bool readingFile = string_contains_char(mode, 'r');
  fn[i++] = readingFile ? 'R' : 'W';
  fn[i] = 0;
  SETNAM(fn, i);
  SETLFS(fileHandleCounter, 8, fileHandleCounter); // todo file ID
  uint8_t error = OPEN();
  if (error >= 20 && error <= 74)
  {
    printf("FOPEN FAILED, ERROR %d\n", (int)error);
    return 0;
  }

  if (readingFile) CHKIN(fileHandleCounter);
  else CHKOUT(fileHandleCounter);

  if ((error = READST()))
  {
    CLOSE(fileHandleCounter);
    CLRCHN();
    printf("READST AFTER FOPEN FAILED, ERROR %d\n", (int)error);
    return 0;
  }

  FILE *f = (FILE*)malloc(sizeof(FILE));
  if (!f)
  {
    CLRCHN();
    printf("FOPEN-MALLOC FAILED!\n");
    return 0;
  }
  if (readingFile) f->nextChar = CHRIN(); // read first char in advance
  f->handle = fileHandleCounter++; // TODO multiple handles
  if (fileHandleCounter == 255) fileHandleCounter = 2;

  return f;
}

int fclose(FILE *f)
{
  assert(f); // The behavior is undefined if the value of the pointer stream is used after fclose returns.
  CLOSE(f->handle);
  CLRCHN();
  free(f);
  return 0;
}

static void consume_read_whitespace(FILE *f)
{
  while(isspace(f->nextChar))
  {
    if (READST()) return;// { CLRCHN(); printf("CONSUMEWS READST ERROR!\n"); return; }
    f->nextChar = CHRIN();
  }
}

static int read_int_from_file(FILE *f)
{
  assert(isdigit(f->nextChar));
  char intstr[6] = { 0, 0, 0, 0, 0, 0 }; // max length 65536
  uint8_t i = 0;
  while(isdigit(f->nextChar))
  {
    intstr[i++] = f->nextChar;
    if (READST()) break;//{ CLRCHN(); printf("READINT READST ERROR!\n"); break; }
    f->nextChar = CHRIN();
    if (i >= 5) i = 0; // if i gets this large, read will overflow (will read garbage)
  }
  return atoi(intstr);
}

static void read_string_from_file(FILE *f, char *dst)
{
  assert(!isspace(f->nextChar));
  while(!isspace(f->nextChar) && !READST())
  {
    *dst++ = f->nextChar;
    f->nextChar = CHRIN();
  }
}

static int fscanf(FILE *f, const char *format, ...)
{
  CHKIN(f->handle);

  va_list args;
  va_start(args, format);
  // consume any initial whitespace in input
  int8_t num_parsed = 0;
  uint8_t ch, error = 0;
  consume_read_whitespace(f);

  while(*format)
  {
    if (isspace(*format))
    {
      consume_read_whitespace(f);
      while(isspace(*format)) ++format;
    }
    else if (*format != '%')
    {
      if (f->nextChar != *format) goto end;
      if ((error = READST())) goto end;
      f->nextChar = CHRIN();
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
    ++format;
  }
end:
  va_end(args);
  CLRCHN();
  if (error) printf("FSCANF ERROR %d\n", (int)error);
  return num_parsed;
}

size_t fread(void *buffer, size_t size, size_t count, FILE *f)
{
  CHKIN(f->handle);  
  uint8_t error = 0;
  uint8_t *ptr = (uint8_t*)buffer;
  for(size_t i = 0; i < size; ++i)
    for(size_t j = 0; j < count; ++j)
    {
      *ptr++ = f->nextChar;
      if ((error = READST())) break;
      f->nextChar = CHRIN();
    }
  CLRCHN();
//////  if (error) printf("FREAD ERROR %d\n", (int)error);
  return (uintptr_t)ptr - (uintptr_t)buffer;
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *f)
{
  CHKOUT(f->handle);
  uint8_t error = 0;
  const uint8_t *ptr = (const uint8_t*)buffer;
  for(size_t i = 0; i < size; ++i)
    for(size_t j = 0; j < count; ++j)
    {
      if ((error = READST())) break;
      CHROUT(*ptr++);
    }
  CLRCHN();
  if (error) printf("FWRITE ERROR %d\n", (int)error);
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

int rrnet_init()
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

void start_tx(uint16_t numBytes)
{
  write_u16_lo_hi(cs_tx_cmd, 0x00C0); // Send Transmit command 00C0h
  write_u16_lo_hi(cs_tx_len, numBytes);
  cs_set_page(0x0138);
  while (!(cs_read_page_data() & 0x0100))
    ; // Wait until we are ready to transmit
}

void tx_bytes(const void *msg, uint16_t numBytes)
{
  numBytes = (numBytes+1)>>1;
  const uint16_t *m16 = (const uint16_t*)msg;
  while(numBytes--)
    write_u16_lo_hi(cs_rxtx_data, *m16++);
}

static volatile unsigned char * const screen_mem = (unsigned char*)0x0400;
static volatile unsigned char * const screen_frame = (unsigned char *)0xD020;

unsigned char ascii_to_petscii(unsigned char ch)
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

unsigned char petscii_to_screen_code(unsigned char petscii)
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
void put_screen(char ch)
{
  screen_mem[screen_pos++] = petscii_to_screen_code(ascii_to_petscii(ch));
  if (screen_pos >= 40*25) screen_pos = 0;
  *screen_frame = ch;
}

void clear_screen()
{
  memset((void*)screen_mem, 0x20, 40*25);
}

void set_cursor_pos(uint8_t x, uint8_t y)
{
  // https://sta.c64.org/cbm64krnfunc.html:
  // $E50A == Save (get) or restore (set) cursor position.
  // Disassembly at https://skoolkid.github.io/sk6502/c64rom/asm/E50A.html
  asm volatile(R"(CLC // Clear Carry: Carry flag 0 signals to the KERNAL subroutine that it should set the cursor position
                  JSR $E50A)"
              :: "x"(y), "y"(x) : "a", "c", "memory");
}

void close_file()
{
  CLOSE(2);
  CLRCHN();
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
  SETNAM(fn, i);
  SETLFS(2, 8, 2);
  uint8_t error = OPEN();
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
  CHKOUT(2);  
  error = READST();

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
          if (READST()) { error = true; goto end; }
          CHROUT(ch[i]);
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
  set_cursor_pos(0, 0);
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
