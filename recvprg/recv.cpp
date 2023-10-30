#include <stdio.h>
#include <string.h>
#include <stdint.h>

static volatile uint8_t * const rrnet_ctl      = (uint8_t *)0xDE01;
static volatile uint8_t * const cs_packet_page = (uint8_t *)0xDE02;
static volatile uint8_t * const cs_packet_data = (uint8_t *)0xDE04;
static volatile uint8_t * const cs_rxtx_data   = (uint8_t *)0xDE08;
static volatile uint8_t * const cs_tx_cmd      = (uint8_t *)0xDE0C;
static volatile uint8_t * const cs_tx_len      = (uint8_t *)0xDE0E;

static __inline__ __attribute__((__always_inline__, __nodebug__)) void write_u16_lo_hi(volatile uint8_t *dst, uint16_t value)
{
  dst[0] = (uint8_t)value;
  dst[1] = (uint8_t)(value>>8);
}

static __inline__ __attribute__((__always_inline__, __nodebug__)) uint16_t read_u16_lo_hi(volatile uint8_t *ptr)
{
  uint8_t lo = ptr[0];
  return (((uint16_t)ptr[1]) << 8) | lo;
}

static __inline__ __attribute__((__always_inline__, __nodebug__)) uint16_t read_u16_hi_lo(volatile uint8_t *ptr)
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
  if (cs_read_page_data() != 0x630E) return 0;

  // Configure chip
  cs_write_page_data(0x0114/*pp_self_ctl*/, 0x0055); // reset chip
  uint16_t mode = 0x0D05; // accept individual and broadcast packets
  if (enable_promiscuous_mode) mode |= 0x80; // and maybe enable promiscuous mode
  cs_write_page_data(0x0104/*pp_rx_ctl*/, mode);
  cs_write_page_data(0x0112/*pp_line_ctl*/, 0xD3); // Enable RX and TX
  return 1;
}

#define ETHERNET_FRAME_SIZE 14
#define IP_FRAME_SIZE 20
#define UDP_HEADER_SIZE 8

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
  uint8_t clob_a, clob_x;
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
  )":"=a"(clob_a),"=x"(clob_x):"a"((uint8_t)numBytes), "x"((uint8_t)(numBytes>>8)):"y","c");
#endif
}

static void start_tx(uint16_t numBytes)
{
}

static volatile unsigned char * const screen_frame = (unsigned char *)0xD020;

uint16_t recv_udp_file_transfer_fragment(bool first_fragment)
{
  // Poll if there is an Ethernet frame ready for processing
  cs_set_page(0x0124);
  while((cs_read_page_data() & 0x0D00) == 0);

  rx_data_hi_lo(); // dummy read to flush status word (ignored)
  uint16_t bytes_left = rx_data_hi_lo();

  const uint8_t min_size = ETHERNET_FRAME_SIZE + IP_FRAME_SIZE + UDP_HEADER_SIZE + 4;
  if (bytes_left < min_size) goto end;
  bytes_left -= min_size;

  for(int i = 0; i < 6; ++i) rx_data_lo_hi();
  uint16_t eth_type;
  eth_type = rx_data_lo_hi();
  for(int i = 0; i < 4; ++i) rx_data_lo_hi();
  uint8_t protocol;
  protocol = rx_data_lo_hi()>>8;
  for(int i = 0; i < 9; ++i) rx_data_lo_hi();
  uint16_t lo;
  uint32_t magic;
  lo = rx_data_lo_hi();
  magic = ((uint32_t)rx_data_lo_hi() << 16) | lo;

  if (eth_type == 0x0008/*IPV4*/ && protocol == 0x11/*UDP*/ &&
     ((first_fragment && magic == RECV_FILE_DATAGRAM_MAGIC && bytes_left == sizeof(recv_file_datagram_header))
    || (!first_fragment && magic == RECV_FILE_DATAGRAM_FRAGMENT_MAGIC && bytes_left > sizeof(recv_file_datagram_fragment_header))))
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

void start_udp_send(uint16_t numBytes)
{
  write_u16_lo_hi(cs_tx_cmd, 0x00C0); // Send Transmit command 00C0h
  write_u16_lo_hi(cs_tx_len, ETHERNET_FRAME_SIZE + IP_FRAME_SIZE + UDP_HEADER_SIZE + numBytes);
  cs_set_page(0x0138);
  while (!(cs_read_page_data() & 0x0100)) ; // Wait until we are ready to transmit

  // Perform UDP broadcast
  write_u16_lo_hi(cs_rxtx_data, 0xFFFF); // dst mac 0,1
  write_u16_lo_hi(cs_rxtx_data, 0xFFFF); // dst mac 2,3
  write_u16_lo_hi(cs_rxtx_data, 0xFFFF); // dst mac 4,5
  write_u16_lo_hi(cs_rxtx_data, 0xCD28); // src mac 0,1
  write_u16_lo_hi(cs_rxtx_data, 0xFF4C); // src mac 2,3
  write_u16_lo_hi(cs_rxtx_data, ntohs(*(uint16_t *)0xDE0E)); // src mac 4,5
  write_u16_lo_hi(cs_rxtx_data, 0x0008); // EtherType

  uint16_t cs = 0;
  cs = sum1c(cs, 0x8011); // TTL + protocol
  cs = sum1c(cs, 0xC0A8); // 192.168
  cs = sum1c(cs, 0x00FF); // 0.255
  cs = sum1c(cs, 0xFFFF); // 255.255
  cs = sum1c(cs, 0xFFFF); // 255.255
  cs = sum1c(cs, 0x4500);
  uint16_t len = IP_FRAME_SIZE + UDP_HEADER_SIZE + numBytes;
  cs = sum1c(cs, len);

  write_u16_lo_hi(cs_rxtx_data, 0x0045); // versionAndHeaderLength
  write_u16_lo_hi(cs_rxtx_data, ntohs(len)); // IP packet length
  write_u16_lo_hi(cs_rxtx_data, 0); // identification (0, unused)
  write_u16_lo_hi(cs_rxtx_data, 0); // flagsAndFragmentOffset (0, unused)
  write_u16_lo_hi(cs_rxtx_data, 0x1180); // TTL (128) + Protocol (17 - UDP)
  write_u16_lo_hi(cs_rxtx_data, ~ntohs(cs)); // Checksum

  write_u16_lo_hi(cs_rxtx_data, 0xA8C0); // src ip 0,1
  write_u16_lo_hi(cs_rxtx_data, 0xFF00); // src ip 2,3
  write_u16_lo_hi(cs_rxtx_data, 0xFFFF); // dst ip 0,1
  write_u16_lo_hi(cs_rxtx_data, 0xFFFF); // dst ip 2,3

  write_u16_lo_hi(cs_rxtx_data, 0); // src port (0, unused)
  write_u16_lo_hi(cs_rxtx_data, ntohs(4019)); // dst port
  write_u16_lo_hi(cs_rxtx_data, ntohs(UDP_HEADER_SIZE + numBytes)); // UDP packet length
  write_u16_lo_hi(cs_rxtx_data, 0); // checksum (0, unused)
  // The only thing left now is to fill the actual UDP payload data,
  // which is the caller's side to do.
}

#define FRAGMENT_RECEIVED_DATAGRAM_MAGIC 0xEE365F02
#define FRAGMENT_RECEIVED_DATAGRAM_SIZE 6

void send_udp_fragment_received_msg(uint16_t fragment_index)
{
  start_udp_send(FRAGMENT_RECEIVED_DATAGRAM_SIZE);
  write_u16_lo_hi(cs_rxtx_data, (uint16_t)FRAGMENT_RECEIVED_DATAGRAM_MAGIC);
  write_u16_lo_hi(cs_rxtx_data, FRAGMENT_RECEIVED_DATAGRAM_MAGIC>>16);
  write_u16_lo_hi(cs_rxtx_data, fragment_index);
}

int main()
{
  rrnet_init(/*promiscuous:*/false);
  uint16_t bytes_left = recv_udp_file_transfer_fragment(true);

  recv_file_datagram_header header;
  read_struct(&header, sizeof(header));

  uint16_t next_fragment_index = 0;

  send_udp_fragment_received_msg(0xFFFF); // Send back msg that we are ready to start transfer.

  while(next_fragment_index != header.num_fragments)
  {
    uint16_t data_left = recv_udp_file_transfer_fragment(false);

    recv_file_datagram_fragment_header frag;
    read_struct(&frag, sizeof(frag));
    data_left -= sizeof(frag);

    if (frag.file_id == header.file_id && frag.fragment_index == next_fragment_index)
    {
      while(data_left > 0)
      {
        uint16_t data = rx_data_lo_hi();
        --data_left;
        if (data_left) --data_left;
        *screen_frame = data;
      }
      send_udp_fragment_received_msg(frag.fragment_index);
      ++next_fragment_index;
    }
    else
      discard_rx_bytes(data_left);
  }
}
