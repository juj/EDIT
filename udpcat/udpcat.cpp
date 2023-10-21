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
    BEQ high_part // Jump to deal directly with high part

    ROR A         // Calculate (A+1)/2 by shifting once right, and
    ADC #0        // adding the lowest bit (now in Carry) back to A.
    TAY           // Move A to Y, since A cannot be used as a loop counter
loop_lo:          // Loop Y times to read the $DE09 hardware port
    LDA $DE09     // Discards one uint16 from CS8900A receive buffer
    DEY
    BNE loop_lo

high_part:        // Now discard full multiples of 256 bytes
    CPX #00       // No high part to discard
    BEQ end

loop_hi:
    LDY #128      // 256 bytes == 128 uint16s
times_128:        // Loop just like before
    LDA $DE09     // Discard a uint16 from CS8900A receive bufer
    DEY
    BNE times_128
   
    DEX
    BNE loop_hi   // If we still have multiples of 256 left, do it again
end:
  )"::"a"((uint8_t)numBytes), "x"((uint8_t)(numBytes>>8)));
#endif
}

void poll_for_datagram()
{
  // Poll if there is an Ethernet frame ready for processing
  cs_set_page(0x0124);
  if ((cs_read_page_data() & 0x0D00) == 0) return;

  rx_data_hi_lo(); // dummy read to flush status word (ignored)
  uint16_t bytes_left = rx_data_hi_lo();

  union
  {
    eth_frame frame;
    ip_frame ip;
    udp_header udp;
  } net;

#define READ_STRUCT(s) \
  if (bytes_left < sizeof(s)) goto end; \
  bytes_left -= sizeof(s); \
  read_struct(&s, sizeof(s));

  READ_STRUCT(net.frame);
  if (net.frame.etherType != 0x0008/*IPV4*/)
  {
    printf("ETH FRAME TYPE: %04XH\n", ntohs(net.frame.etherType));
    goto end;
  }

  READ_STRUCT(net.ip);
  if (net.ip.protocol != 0x11/*UDP*/)
  {
    printf("IP FRAME PROTO: %02XH\n", (int)net.ip.protocol);
    goto end;
  }

  READ_STRUCT(net.udp);
  net.udp.src_port = ntohs(net.udp.src_port);
  net.udp.dst_port = ntohs(net.udp.dst_port);
  net.udp.length = ntohs(net.udp.length);

  printf("UDP:%d.%d.%d.%d -> %d.%d.%d.%d\n",
    net.ip.src_ip[0], net.ip.src_ip[1], net.ip.src_ip[2], net.ip.src_ip[3],
    net.ip.dst_ip[0], net.ip.dst_ip[1], net.ip.dst_ip[2], net.ip.dst_ip[3]);
  printf("    PORT:%u->%u, LEN:%u\n   ", net.udp.src_port, net.udp.dst_port, net.udp.length);
  for(int i = 0; i < 6 && bytes_left >= 2; ++i)
  {
    uint16_t data = rx_data_lo_hi();
    printf(" %02X %02X", data>>8, data&0xFF);
    bytes_left -= 2;
  }
  printf("\n");

end:
  // Flush rest of the received bytes from the receive buffer.
  // TODO: is there a way to tell CS8900A to do this? This takes up precious cycles.
  discard_rx_bytes(bytes_left);
}

static volatile unsigned char * const queued_key_buffer_length = (unsigned char *)0x00C6;

bool kbhit()
{
  return *queued_key_buffer_length != 0;
}

int main()
{
  bool promiscuous = false;
  if (!rrnet_init(promiscuous))
    return 1;

  while(kbhit()) _GETIN();
  bool quit = false;
  printf("LISTENING FOR UDP TRAFFIC. ESC QUITS.\n");
  printf("PROMISCUOUS MODE DISABLED. F1 TOGGLES.\n");
  while(!quit)
  {
    poll_for_datagram();
    if (kbhit())
      switch(_GETIN())
      {
      case 0x5F: quit = true; break; // ESC (left arrow) key
      case 0x85:
        promiscuous = !promiscuous;
        rrnet_init(promiscuous);
        printf("SWITCHED TO %s MODE\n", promiscuous ? "PROMISCUOUS" : "NORMAL");
        break;
      }
  }
  while(kbhit()) _GETIN();
}
