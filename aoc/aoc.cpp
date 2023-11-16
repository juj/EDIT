#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../c64/c64_kernal.h"
#include "../c64/c64_cursor.h"

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

int rrnet_init(bool enable_promiscuous_mode)
{
  *rrnet_ctl |= 1; // enable clock port on the C64 to access RR-Net through a carrier cartridge

  // Detect RR-Net
  cs_set_page(0);
  if (cs_read_page_data() != 0x630E)
  {
    printf("RR-NET/CS8900A WAS NOT DETECTED.\n");
    return 0;
  }
/*
  // Detect RR-Net MK3 by calculating a checksum. See http://wiki.icomp.de/wiki/RRNet
  volatile uint8_t * const mk3_eeprom = (uint8_t *)0xDE0C;
  if ((mk3_eeprom[0] ^ mk3_eeprom[1] ^ 0x55) != mk3_eeprom[2] ||
     ((mk3_eeprom[0] + mk3_eeprom[1] + mk3_eeprom[2]) ^ 0xAA) != mk3_eeprom[3])
  {
    printf("RR-NET MK3 NOT DETECTED (CKSUM ERROR)\n");
  }
*/
  // Configure chip
  cs_write_page_data(0x0114/*pp_self_ctl*/, 0x0055); // reset chip
  uint16_t mode = 0x0D05; // accept individual and broadcast packets
  if (enable_promiscuous_mode) mode |= 0x80; // and maybe enable promiscuous mode
  cs_write_page_data(0x0104/*pp_rx_ctl*/, mode);
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
  uint16_t file_start_address;
  uint16_t prg_entry_func_address;
  uint16_t file_size;
  uint8_t dummy0;
  uint8_t filetype;
} __attribute__((packed));

#define RECV_FILE_DATAGRAM_FRAGMENT_MAGIC 0x25291424
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
  // Read the hardware port cs_rxtx_data[1] numBytes/2 times (rounded up)
  // to discard received uint16s that are uninteresting.
#if 0
  numBytes = (numBytes+1)>>1;
  while(numBytes-- > 0)
    (void)cs_rxtx_data[1];
#else
  // This is a hot function, so implement a tight discard loop
  // manually.
  asm(R"(
    // A and X contain number of bytes to discard
    CMP #0        // Test if A is zero (have an even multiple of 256 bytes to discard)
    BEQ high_part%= // Jump to deal directly with high part

    ROR A         // Calculate (A+1)/2 by shifting once right, and
    ADC #0        // adding the lowest bit (now in Carry) back to A.
    TAY           // Move A to Y, since A cannot be used as a loop counter
loop_lo%=:        // Loop Y times to read the $DE09 hardware port
    LDA $DE09     // Discards one uint16 from CS8900A receive buffer
    DEY
    BNE loop_lo%=

high_part%=:      // Now discard full multiples of 256 bytes
    CPX #00       // No high part to discard
    BEQ end%=

loop_hi%=:
    LDY #128      // 256 bytes == 128 uint16s
times_128%=:      // Loop just like before
    LDA $DE09     // Discard a uint16 from CS8900A receive bufer
    DEY
    BNE times_128%=
   
    DEX
    BNE loop_hi%= // If we still have multiples of 256 left, do it again
end%=:
  )"::"a"((uint8_t)numBytes), "x"((uint8_t)(numBytes>>8)));
#endif
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
static volatile unsigned char * const color_mem = (unsigned char*)0xD800;
static volatile unsigned char * const screen_border = (unsigned char *)0xD020;

static unsigned char ascii_to_business_petscii(unsigned char ch)
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
  if (petscii == 0x0D) petscii = 0xFF; // Route newline to 0xFF
  if (petscii <= 31) return 32;//petscii + 128;
  if (petscii <= 63) return petscii;
  if (petscii <= 95) return petscii - 64;
  if (petscii <= 127) return petscii - 32;
  if (petscii <= 159) return 32;//petscii + 64;
  if (petscii <= 191) return petscii - 64;
  if (petscii <= 254) return petscii - 128;
  return 0x5E;
}

static void put_screen(int screen_pos, char ch, unsigned char color, uint8_t highlight)
{
  screen_mem[screen_pos] = petscii_to_screen_code(ch) ^ highlight;
  color_mem[screen_pos] = color;
}

static void clear_screen()
{
  _CLRSCR();
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
  char file_type = 'S';
  if (strstr(filename, ".PRG") || strstr(filename, ".prg"))
  {
    i -= 4; // Erase ".PRG" part from file suffix
    file_type = 'P';
  }
  fn[i++] = ',';
  fn[i++] = file_type;
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

uint16_t sum1c(uint16_t a, uint16_t b)
{
  a += b;
  return a + (a < b);
}

uint16_t calculate_one1c_checksum(void *data, uint16_t numBytes)
{
  assert(numBytes % 2 == 0);
  numBytes >>= 1;
  uint16_t *d = (uint16_t *)data;
  uint16_t checksum = 0;
  while(numBytes--)
    checksum = sum1c(checksum, ntohs(*d++));
  return ~checksum;
}

void start_udp_send(uint16_t numBytes)
{
  union
  {
    eth_frame frame;
    ip_frame ip;
    udp_header udp;
  } net;

  uint16_t numBytesRoundedUpToEven = numBytes + (numBytes&1);
  start_tx(sizeof(net.frame) + sizeof(net.ip) + sizeof(net.udp) + numBytesRoundedUpToEven);

  // Perform UDP broadcast
  for(int i = 0; i < 6; ++i)
    net.frame.dst_mac[i] = 0xFF;

  // http://wiki.icomp.de/wiki/RRNet
  net.frame.src_mac[0] = 0x28;
  net.frame.src_mac[1] = 0xCD;
  net.frame.src_mac[2] = 0x4C;
  net.frame.src_mac[3] = 0xFF;
  net.frame.src_mac[4] = *(volatile unsigned char *)0xDE0E;
  net.frame.src_mac[5] = *(volatile unsigned char *)0xDE0F;

  net.frame.etherType = 0x0008;
  tx_bytes(&net.frame, sizeof(net.frame));

  net.ip.versionAndHeaderLength = 0x45;
  net.ip.typeOfService = 0;
  net.ip.packetLength = ntohs(sizeof(net.ip) + sizeof(net.udp) + numBytesRoundedUpToEven);
  net.ip.identification = 0;
  net.ip.flagsAndFragmentOffset = 0;
  net.ip.timeToLive = 128;
  net.ip.protocol = 17; // udp
  net.ip.headerChecksum = 0; // temporary, will be filled later

  net.ip.src_ip[0] = 192;
  net.ip.src_ip[1] = 168;
  net.ip.src_ip[2] = 0;
  net.ip.src_ip[3] = 255;

  net.ip.dst_ip[0] = 255;
  net.ip.dst_ip[1] = 255;
  net.ip.dst_ip[2] = 255;
  net.ip.dst_ip[3] = 255;
  net.ip.headerChecksum = ntohs(calculate_one1c_checksum(&net.ip, sizeof(net.ip))); // Calculate the IP checksum (this is not optional)

  tx_bytes(&net.ip, sizeof(net.ip));

  net.udp.src_port = ntohs(49739);
  net.udp.dst_port = ntohs(4009);
  net.udp.length = ntohs(sizeof(net.udp) + numBytesRoundedUpToEven);
  net.udp.checksum = 0; // Skip computing UDP checksum - that is optional. (tricky to compute since checksum extends over user payload)
  tx_bytes(&net.udp, sizeof(net.udp));

  // The only thing left now is to fill the actual UDP payload data,
  // which is the caller's side to do.
}

#define FRAGMENT_RECEIVED_DATAGRAM_MAGIC 0xEE365F02
struct fragment_received
{
  uint32_t magic;
  uint16_t fragment_index;
};

void send_udp_fragment_received_msg(uint16_t fragment_index)
{
  fragment_received recv;
  recv.magic = FRAGMENT_RECEIVED_DATAGRAM_MAGIC;
  recv.fragment_index = fragment_index;
//  printf("ACKING %u\n", recv.fragment_index);
  start_udp_send(sizeof(recv));
  tx_bytes(&recv, sizeof(recv));
}

#define MAGIC_ASK_AOC_CALENDAR 0x39DE312B
struct ask_aoc_calendar
{
  uint32_t magic;
  int year;
};

void send_ask_aoc_calendar(int year)
{
  ask_aoc_calendar msg = {
    .magic = MAGIC_ASK_AOC_CALENDAR,
    .year = year
  };
  start_udp_send(sizeof(msg));
  tx_bytes(&msg, sizeof(msg));
}

#define MAGIC_ASK_AOC_DAY 0x910532EF
struct ask_aoc_day
{
  uint32_t magic;
  int year;
  unsigned char day;
  unsigned char line_width;
};

void send_ask_aoc_day(int year, int day, int line_width)
{
  ask_aoc_day msg = {
    .magic = MAGIC_ASK_AOC_DAY,
    .year = year,
    .day = (unsigned char)day,
    .line_width = (unsigned char)line_width
  };
  start_udp_send(sizeof(msg));
  tx_bytes(&msg, sizeof(msg));
}

#define MAGIC_ASK_AOC_INPUT 0x12985331
struct ask_aoc_input
{
  uint32_t magic;
  int year;
  unsigned char day;
  unsigned char line_width;
};

void send_ask_aoc_input(int year, int day, int line_width)
{
  ask_aoc_input msg = {
    .magic = MAGIC_ASK_AOC_INPUT,
    .year = year,
    .day = (unsigned char)day,
    .line_width = (unsigned char)line_width
  };
  start_udp_send(sizeof(msg));
  tx_bytes(&msg, sizeof(msg));
}

//char rrnet_recv_filename[MAX_FILENAME_LEN+1];

char *receive_file_in_memory()
{
  _CLRSCR();
//  printf("RECEIVE FILE IN MEMORY\n");
  uint16_t bytes_left = 0;
  while(!bytes_left) bytes_left = poll_for_udp_file_transfer_fragment(true);
  recv_file_datagram_header header;
  assert(bytes_left == sizeof(header));
  read_struct(&header, sizeof(header));

  uint8_t orig_border = *screen_border;
  uint16_t next_fragment_index = 0;
  char *recv_data = (char*)malloc(header.file_size+2);
  assert(recv_data);
  if (!recv_data) return 0;
  uint16_t file_offset = 0;
//  printf("FILE SIZE: %u\n", header.file_size);
  send_udp_fragment_received_msg(0xFF); // Send back msg that we are ready to start transfer.

  while(next_fragment_index != header.num_fragments)
  {
    uint16_t data_left = 0;
    while(!data_left) data_left = poll_for_udp_file_transfer_fragment(false);

    recv_file_datagram_fragment_header frag;
    read_struct(&frag, sizeof(frag));
    data_left -= sizeof(frag);
//    printf("PAYLOAD: %u BYTES\n", data_left);

    if (frag.file_id == header.file_id)
    {
      while(data_left > 1)
      {
        uint16_t data = rx_data_lo_hi();
//        if (file_offset < 80) printf("%04X ", data);
//        _CHROUT(data);
  //      _CHROUT(data>>8);
        recv_data[file_offset] = data;
        recv_data[file_offset+1] = data>>8;
//        *(uint16_t*)(recv_data+file_offset) = data;//
        *screen_border = data;
        file_offset += 2;
        data_left -= 2;
      }
      if (data_left) // trailing odd byte?
      {
        recv_data[file_offset++] = (uint8_t)rx_data_lo_hi();
        --data_left;
      }
      assert(file_offset <= header.file_size+1);
//      printf("FILE_OFFSET=%u\n", file_offset);
      send_udp_fragment_received_msg(frag.fragment_index);
      ++next_fragment_index;
    }
    else
      discard_rx_bytes(data_left);
  }

end:
  *screen_border = orig_border;
  recv_data[file_offset] = 0;
  recv_data[header.file_size] = 0;
  /*
  for(int i = 0; i < 80; ++i)
    printf("%02X ", recv_data[i]);
  printf("%s\n", recv_data);
  */
  //printf("TRANSFER FINISHED!\n");
  return recv_data;
}

int count_num_lines(const char *str)
{
  int c = 1;
  while(*str) c += (*str++ == '\n');
  return c;
}

uint8_t get_key()
{
  uint8_t key = 0;
  while(!key) key = _GETIN();
  return key;
}

int *build_col_start_addresses(const char *str, int num_lines)
{
  int *starts = (int*)malloc((num_lines+1)*sizeof(int));
  if (!starts)
  {
    malloc_dump_heap();
  }
  assert(starts);
  if (!starts) return 0;
  int cur = 0;
  starts[0] = 0;
  const char *s = str;
  for(; *s; ++s)
    if (*s == '\n')
      starts[++cur] = s - str + 1;
  starts[num_lines] = s + 1 - str;
  return starts;
}

int line_length(const char *line)
{
  int len = 0;
  while(*line && *line != '\n' && *line != '\r') if (*line++ < 128) ++len;
  return len;
}

int longest_line_length(const char *text, int *starts, int num_lines)
{
  int max = 0;
  for(int i = 0; i < num_lines; ++i)
  {
    int len = line_length(text + starts[i]);
    if (max < len) max = len;
  }
  return max;
}
/*
int find_line_idx_for_day(char *calendar, int *starts, int num_lines, int day, int cur_line_idx)
{
  char s[6];
  sprintf(s, " %d ", day);

  if (cur_line_idx > 0 && strstr(calendar[starts[cur_line_idx-1]], s)) return cur_line_idx-1;
  if (cur_line_idx+1 < num_lines && strstr(calendar[starts[cur_line_idx+1]], s)) return cur_line_idx+1;
  for(int i = 0; i < num_lines; ++i)
    if (strstr(calendar[starts[i]], s)) return i;
  return 0;
}
*/

extern "C" size_t __heap_start;

uint8_t SCREEN_WIDTH = 40;
uint8_t SCREEN_HEIGHT = 25;

void toggle_highlight_on_line(int y)
{
/*
  for(volatile unsigned char *start = screen_mem + y*40,
    *end = start + 40; start != end; ++start)
    *start ^= 0x80;
*/
  for(int x = 0; x < SCREEN_WIDTH; ++x)
    screen_mem[y*SCREEN_WIDTH+x] ^= 0x80;
}

int parse_selected_puzzle_day(const char *str, int line_length)
{
  _CLRSCR();
//  printf("PARSE: %s\n", str);
  const char *x = str + line_length;//(strlen(str)-1);
  while(x > str && !isdigit(*x)) --x;
  while(x > str && isdigit(*x)) --x;
  ++x;
//  printf("PARSING \"%s\"\n", x);
//  printf("GOT %d\n", atoi(x));
  return atoi(x);
}
/*
void convert_to_screen_codes(char *str)
{
  while(*str)
  {
    *str = petscii_to_screen_code(ascii_to_petscii(*str));
    ++str;
  }
}
*/
void convert_to_business_petscii(char *str)
{
  while(*str)
  {
    *str = ascii_to_business_petscii(*str);
    ++str;
  }
}

#define BLACK       0
#define WHITE       1
#define RED         2
#define CYAN        3
#define MAGENTA     4
#define GREEN       5
#define BLUE        6
#define YELLOW      7
#define ORANGE      8
#define BROWN       9
#define LTRED      10
#define BLACK_GRAY 11
#define DKGRAY     12
#define LTGREEN    13
#define LTBLUE     14
#define LTGRAY     15

void clear_line(uint16_t screenPos, uint8_t ch, uint8_t color)
{
  for(uint8_t i = 0; i < SCREEN_WIDTH; ++i)
    put_screen(screenPos + i, ch, color, 0);
}

void put_string_on_screen(const char *str, uint8_t x, uint8_t y, uint8_t color)
{
  uint16_t pos = (uint16_t)y*SCREEN_WIDTH + x;
  for(uint8_t i = 0; str[i] != 0; ++i, ++pos)
    put_screen(pos, str[i], color, 0);
}

int ask_string_prompt(const char *prompt, char *dst, int maxLen)
{
  clear_line((uint16_t)(SCREEN_HEIGHT-1)*SCREEN_WIDTH, ' ', LTGRAY); // Clear last line to start with

  put_string_on_screen(prompt, 0, SCREEN_HEIGHT-1, WHITE);
  uint8_t prompt_len = (uint8_t)strlen(prompt);
  uint8_t x = (uint8_t)strlen(dst);
  put_string_on_screen(dst, prompt_len, SCREEN_HEIGHT-1, LTGRAY);
  const uint16_t pos = (uint16_t)(SCREEN_HEIGHT-1)*SCREEN_WIDTH+prompt_len;
  int8_t result = 0;
  while(result == 0)
  {
    int key = get_key();
    switch(key)
    {
    case 0x5F: result = -1; break;
    case 0x0D: if (x > 0) result = 1; break;
    case 0x14:
      if (x > 0)
      {
        dst[--x] = 0;
        put_screen(pos+x, ' ', LTGRAY, 0);
        c64_set_cursor_pos(prompt_len+x, SCREEN_HEIGHT-1);
      }
      break;
    default:
      if (x < maxLen && key >= 0x20 && key < 127)
      {
        if (maxLen > 1 || key == 'y' || key == 'Y' || key == 'n' || key == 'N')
        {
          c64_set_cursor_pos(prompt_len+x+1, SCREEN_HEIGHT-1);
          put_screen(pos+x, key, LTGRAY, 0);
          dst[x++] = key;
        }
      }
      break;
    }
  }
  dst[x] = 0;
  if (result < 0) return -1;
  return x;
}

#define MAGIC_SUBMIT_AOC_ANSWER 0x4291FF01
struct submit_aoc_answer
{
  uint32_t magic;
  int year;
  unsigned char day;
  char answer[33];
};

void send_submit_aoc_answer(int year, int day, char *answer)
{
  submit_aoc_answer msg = {
    .magic = MAGIC_SUBMIT_AOC_ANSWER,
    .year = year,
    .day = (unsigned char)day
  };
  strcpy(msg.answer, answer);
  start_udp_send(sizeof(msg));
  tx_bytes(&msg, sizeof(msg));
}

int selected_year = 2023;
int selected_day = 0;

#define VIEW_CALENDAR 0
#define VIEW_DESC 1
#define VIEW_INPUT 2
#define VIEW_RESPONSE 3
int view_file(char *text, uint8_t view_type);

uint8_t remove_control_codes(char *dst, const char *s, uint8_t srcLen)
{
  char *d = dst;
  while(*s && *s != '\n' && *s != '\r' && srcLen--)
  {
    if ((uint8_t)*s < 0x80 || (uint8_t)*s > 0x91)
      *d++ = *s;
    ++s;
  }
  *d = 0;
  return d - dst;
}

void ask_answer()
{
  char answer[31] = {};
  ask_string_prompt("answer: ", answer, sizeof(answer)-1);
  send_submit_aoc_answer(selected_year, selected_day, answer);

  char *response = receive_file_in_memory();
  view_file(response, VIEW_RESPONSE);
}

int view_file(char *text, uint8_t view_type)
{
  int num_lines = count_num_lines(text);

  int *starts = build_col_start_addresses(text, num_lines);
//  --num_lines;
  int max_x = longest_line_length(text, starts, num_lines) - SCREEN_WIDTH;
  int max_y = num_lines - SCREEN_HEIGHT;

//  printf("\\N = %d\n", (int)petscii_to_screen_code(ascii_to_petscii('\n')));
  convert_to_business_petscii(text);
  /*
  char *c = text;
  while(*c) // Delimit lines with zeros
  {
    if (*c == '\n' || *c == '\r') *c = 0;
    ++c;
  }*/
  for(int i = 0; i < num_lines; ++i)
    text[starts[i]-1] = 0;

  bool line_select_mode = false;
  int line_select_start = -1, line_select_end = -1;
  int line_select_first_line = 0;
  int x = (view_type == VIEW_CALENDAR) ? max_x : 0;
  int y = 0;
  int cursor_y = 0;

  bool repaint = true;
  for(;;)
  {
    if (repaint)
    {
      int screen_pos = 0;
      for(uint8_t Y = 0; Y < SCREEN_HEIGHT && y+Y < num_lines; ++Y)
      {
        char *s = text + starts[y+Y];
        uint8_t highlight = (y+Y == cursor_y) ? 0x80 : 0;
        uint8_t extraHighlight = 0;
        uint8_t line_length = starts[y+Y+1] - starts[y+Y] - 1;
        bool cur_line_selected = (y+Y >= line_select_start && y+Y <= line_select_end);
        unsigned char color = cur_line_selected ? 14 : 15;
        uint8_t cur_x = 0;
        while(cur_x < x && cur_x < line_length)
        {
          if (s[cur_x] >= 128 && s[cur_x] < 128 + 16) color = cur_line_selected ? 14 : s[cur_x] - 128;
          else if (s[cur_x] == 0x90) extraHighlight = 0x80;
          else if (s[cur_x] == 0x91) extraHighlight = 0;
          ++cur_x;
        }
        for(uint8_t X = 0; X < SCREEN_WIDTH; ++X)
        {
          unsigned char ch = cur_x < line_length ? s[cur_x] : ' ';
          while(ch >= 128 && ch < 128 + 18)
          {
            if (ch < 128 + 16) { if (!cur_line_selected) color = ch - 128; ch = s[++cur_x]; }
            else if (ch == 128+16) { extraHighlight = 0x80; ch = s[++cur_x]; }
            else /*if (ch == 0x91)*/ { extraHighlight = 0; ch = s[++cur_x]; }
//            else break;
          }
          put_screen(screen_pos++, (ch >= 32) ? ch : ' ', color, highlight ^ extraHighlight);
          ++cur_x;
        }
      }
      repaint = false;
    }

    unsigned char key = 0;
    while(!key) key = _GETIN();
    if (key == 0x11 || key == 0x88 || key == 0x8C)
    {
      toggle_highlight_on_line(cursor_y-y);
      cursor_y += (key == 0x8C) ? SCREEN_HEIGHT-1 : (key == 0x11) ? 1 : 8;
      if (cursor_y >= num_lines) cursor_y = num_lines-1;
      if (cursor_y > y+SCREEN_HEIGHT-1)
      {
        y += cursor_y - (y+SCREEN_HEIGHT-1);
        repaint = true;
      }
      else
        toggle_highlight_on_line(cursor_y-y);
    }
    else if (key == 0x1D && x < max_x) { ++x; repaint = true; }
    else if (key == 0x91 || key == 0x87 || key == 0x8B)
    {
      toggle_highlight_on_line(cursor_y-y);
      cursor_y -= (key == 0x8B) ? SCREEN_HEIGHT-1 : (key == 0x91) ? 1 : 8;
      if (cursor_y < 0) cursor_y = 0;
      if (cursor_y < y)
      {
        y = cursor_y;
        repaint = true;
      }
      else
        toggle_highlight_on_line(cursor_y-y);
    }
    else if (key == 0x9D && x > 0) { --x; repaint = true; }
    else if (view_type == VIEW_CALENDAR && key == 0x0D) // Enter
    {
      char *s = text + starts[cursor_y];
      int day = parse_selected_puzzle_day(s, starts[cursor_y+1] - starts[cursor_y]);
      if (day)
      {
        free(text);
        free(starts);
        return day;
      }
    }
    else if (key == 0x85) // F1
    {
      line_select_mode = !line_select_mode;
      line_select_start = line_select_end = line_select_mode ? cursor_y : -1;
      line_select_first_line = cursor_y;
      repaint = true;
    }
    else if (key == 0x53) // S
    {
      char device[8] = {};
      ask_string_prompt("SAVE TO DEVICE #: ", device, 2);
      strcat(device, "w");
      char filename[17] = {};
      printf("\n"); // Add a newline so that the next question appears on a new line
      ask_string_prompt("ENTER FILENAME: ", filename, 16);
      printf("\n"); // Add a newline so that the text "PRESS RECORD & PLAY ON TAPE" will appear on a line of its own
      int l0 = line_select_mode?line_select_start:0;
      int l1 = line_select_mode?line_select_end+1:num_lines;
      uint16_t num_bytes = 0;
      int line_num = 0;
      //int screen_pos, char ch, unsigned char color, uint8_t highlight)
      put_string_on_screen("saving: [                ]", 0, SCREEN_HEIGHT-1, WHITE);
      int pos = SCREEN_WIDTH*(SCREEN_HEIGHT-1) + strlen("saving: [");
      int progress_incr = num_lines >> 4;
      int progress = 0;
      _Jiffies elapsed = _RDTIM();
      FILE *handle = fopen(filename, device);
      char *tmp = (view_type == VIEW_DESC) ? (char*)malloc(82) : 0;
      for(int line = l0; line < l1; ++line)
      {
        if (++progress >= progress_incr)
        {
          progress -= progress_incr;
          put_screen(pos++, '*', 1, 0);
        }
        char *s = text+starts[line];
        int len = starts[line+1] - starts[line] - 1;
        assert(len <= 255);

        if (view_type == VIEW_DESC)
        {
          uint8_t out_len = remove_control_codes(tmp, s, len);
          assert(out_len <= 81);
          fwrite8(tmp, out_len, handle);
        }
        else
          fwrite8(s, (uint8_t)len, handle);
        fputc('\n',handle);
        num_bytes += len + 1;
      }
      free(tmp);
      fclose(handle);
      elapsed = _RDTIM() - elapsed;
      char str[44];
      uint16_t bytes_per_sec = (uint16_t)((unsigned long)num_bytes*60/elapsed);
      sprintf(str, "wrote %d lines/%u bytes", l1-l0, num_bytes);
      put_string_on_screen(str, 0, SCREEN_HEIGHT-1, WHITE);
      uint16_t seconds = (uint16_t)(elapsed / 60);
      //sprintf(str, "in %lu jiffies=%u sec, %u b/sec.", (unsigned long)elapsed, seconds, bytes_per_sec);
      //put_string_on_screen(str, 0, SCREEN_HEIGHT-1, WHITE);
      get_key();
      line_select_mode = false;
      line_select_start = line_select_end = -1;
      repaint = true;
    }
    else if (view_type != VIEW_CALENDAR)
    {
      if (key == 0x5F || (view_type == VIEW_RESPONSE && key == 0x0D)) // ESC
      {
        free(text);
        free(starts);
        return 0;
      }
      else if (key == 0x50) // P
      {
        free(text);
        free(starts);
        return -1; // toggle for printing
      }
      else if (view_type == VIEW_DESC)
      {
        const char *s = text + starts[cursor_y];
        if (key == 0x0D && (strstr(s, "RETURN") || strstr(s, "CALENDAR")))
        {
          free(text);
          free(starts);
          return 0;          
        }
        if (key == 0x49/*I*/ || (key == 0x0D && (strstr(s, "GET") || strstr(s, "INPUT")))) // I shows input
        {
          free(text);
          free(starts);
          return 1;
        }
        if (key == 0x0D && strstr(s, "NSWER:"))
        {
          ask_answer();
          free(text);
          free(starts);
          return -2; // reload this page
        }
      }
    }

    if (line_select_mode)
    {
      line_select_start = cursor_y < line_select_first_line ? cursor_y : line_select_first_line;
      line_select_end = cursor_y < line_select_first_line ? line_select_first_line : cursor_y;
      repaint = true;
    }
  }
}

static volatile unsigned char * const mem_setup_register = (unsigned char *)0xD018;
static volatile unsigned char * const screen_bg = (unsigned char *)0xD021;
static volatile unsigned char * const key_repeat_register = (unsigned char *)0x028A;

int main()
{
  *key_repeat_register |= 0x80; // Make all keys repeat
  *screen_border = 5; // green
  *screen_bg = 0; // black
  _SCREEN(&SCREEN_WIDTH, &SCREEN_HEIGHT);
//  unsigned int heap_size = 0xCD00 - (unsigned int)&__heap_start;
//  __set_heap_limit(heap_size);

  // Switch charset into text mode, where both lower case and upper case characters
  // are available. Default register value at boot is 0x15.
  _CLRSCR();
  *mem_setup_register = 0x17;

  int success = rrnet_init(/*promiscuous:*/false);
  if (!success) printf("RR-NET NOT FOUND!\n");

  bool for_printing = false;
  selected_year = 2020;
  for(;;)
  {
    send_ask_aoc_calendar(selected_year);
    char *calendar = receive_file_in_memory();
    if (!calendar) break;
    selected_day = view_file(calendar, VIEW_CALENDAR);
  //  printf("<:BACK F1:PRINT F3:SAVE INPUT F7:SUBMIT");

    int ret = -1;
    while(ret < 0)
    {
      send_ask_aoc_day(selected_year, selected_day, for_printing ? 80 : SCREEN_WIDTH);
      char *text = receive_file_in_memory();
      if (!text) break;
      ret = view_file(text, VIEW_DESC);
      if (ret == -1) for_printing = !for_printing;
      else if (ret == 1) // view input?
      {
        send_ask_aoc_input(selected_year, selected_day, 250);
        char *text = receive_file_in_memory();
        if (!text) break;
        view_file(text, VIEW_INPUT);
        ret = -1;
      }
    }
  }
}
