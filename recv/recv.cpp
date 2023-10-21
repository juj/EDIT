#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../c64/c64_kernal.h"

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
    printf("RR-NET/CS8900A WAS NOT DETECTED.");
    return 0;
  }

  // Detect RR-Net MK3 by calculating a checksum. See http://wiki.icomp.de/wiki/RRNet
  volatile uint8_t * const mk3_eeprom = (uint8_t *)0xDE0C;
  if ((mk3_eeprom[0] ^ mk3_eeprom[1] ^ 0x55) != mk3_eeprom[2] ||
     ((mk3_eeprom[0] + mk3_eeprom[1] + mk3_eeprom[2]) ^ 0xAA) != mk3_eeprom[3])
  {
    printf("RR-NET MK3 NOT DETECTED (CKSUM ERROR)\n");
  }

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
  start_udp_send(sizeof(recv));
  tx_bytes(&recv, sizeof(recv));
}

char rrnet_recv_filename[MAX_FILENAME_LEN+1];

const char *poll_to_receive_file()
{
  uint16_t bytes_left = poll_for_udp_file_transfer_fragment(true);
  if (!bytes_left)
    return 0;

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

  send_udp_fragment_received_msg(0xFFFF); // Send back msg that we are ready to start transfer.

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
      send_udp_fragment_received_msg(frag.fragment_index);
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

int main()
{
  int success = rrnet_init(/*promiscuous:*/false);
  if (success) printf("RR-NET INITIALIZED\n");
  else printf("RR-NET NOT FOUND!\n");

  bool quit = false;
  while(!quit)
  {
    printf("WAITING TO RECEIVE FILE (ESC QUITS)...\n");
    while(!quit)
    {
      if (poll_to_receive_file()) break;
      if (_KBHIT() && _GETIN() == 0x5F/*ESC*/) quit = true;
    }
  }
  _RESET();
}
