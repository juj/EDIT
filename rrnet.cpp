#include <stdio.h>
#include <stdint.h>

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

static void read_struct(void *dst, uint16_t numBytes)
{
  uint16_t *frame = (uint16_t *)dst;
  numBytes = (numBytes+1)>>1;
  while(numBytes--)
    *frame++ = rx_data_lo_hi();
}

eth_frame frame;
ip_frame ip;
udp_header udp;

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

static void tx_bytes(const char *msg, uint16_t numBytes)
{
  numBytes = (numBytes+1)>>1;
  while(numBytes--)
  {
    cs_rxtx_data[0] = msg[0];
    cs_rxtx_data[1] = msg[1];
    msg += 2;
  }
}

void wait_for_packets()
{
  cs_set_page(0x0124);

  while((cs_read_page_data() & 0x0D00) == 0)
    ;

  rx_data_hi_lo(); // dummy read to flush status word (ignored)
  uint16_t packet_length = rx_data_hi_lo();
//  printf("PACKET LENGTH: %u\n", packet_length);
  if (packet_length >= sizeof(eth_frame))
  {
    read_struct(&frame, sizeof(eth_frame));
    packet_length -= sizeof(eth_frame);
/*
    printf("SRC: %02X:%02X:%02X:%02X:%02X:%02X\n",
      frame.src_mac[0],frame.src_mac[1],frame.src_mac[2],
      frame.src_mac[3],frame.src_mac[4],frame.src_mac[5]);
    printf("DST: %02X:%02X:%02X:%02X:%02X:%02X\n",
      frame.dst_mac[0],frame.dst_mac[1],frame.dst_mac[2],
      frame.dst_mac[3],frame.dst_mac[4],frame.dst_mac[5]);
*/
    switch(frame.etherType)
    {
    case 0x0008: //printf("IPV4\n");
      read_struct(&ip, sizeof(ip));
      if (packet_length >= sizeof(ip)) packet_length -= sizeof(ip);
      else packet_length = 0;
      switch(ip.protocol)
      {
//      case 1/*ICMP*/: printf("- ICMP\n"); break;
      case 0x11/*UDP*/:
        read_struct(&udp, sizeof(udp));
        if (packet_length >= sizeof(udp)) packet_length -= sizeof(udp);
        else packet_length = 0;

        udp.src_port = ntohs(udp.src_port);
        udp.dst_port = ntohs(udp.dst_port);
        udp.length = ntohs(udp.length);

        if (udp.dst_port == 4009)
        {
          printf("SRC IP: %d.%d.%d.%d\n", ip.src_ip[0], ip.src_ip[1], ip.src_ip[2], ip.src_ip[3]);
          printf("DST IP: %d.%d.%d.%d\n", ip.dst_ip[0], ip.dst_ip[1], ip.dst_ip[2], ip.dst_ip[3]);

          printf("UDP: %u->%u, LEN: %u\n", udp.src_port, udp.dst_port, udp.length);
          debug_dump_rx_bytes(packet_length);
          printf("\n");
        }
        break;
//      case 0x6/*TCP*/: printf("- TCP\n"); break;
      default:/* printf("PROTOCOL %d\n", (int)ip.protocol);*/ break;
      }
      break;
//    case 0x0608: printf("ARP\n"); break;
    default: /*printf("%04XH\n", (int)frame.etherType);*/ break;
    } 
  /*
    printf("TYPE: %02XH\n", frame.type);

    int print = 8;
    while(packet_length-- > 0 && print-- > 0)
    {
      uint16_t word = *cs_rxtx_data;
      printf("%04X", word);
    }
    if (packet_length > 0) printf("...\n");
    else printf("\n");
    */
  }

  // Flush rest of the received words
  discard_rx_bytes(packet_length);
}

void send_udp_broadcast(const void *data, uint16_t numBytes)
{
  uint16_t numBytesRoundedUpToEven = numBytes + (numBytes&1);
  start_tx(sizeof(frame) + sizeof(ip) + sizeof(udp) + numBytesRoundedUpToEven);
  for(int i = 0; i < 6; ++i)
    frame.dst_mac[i] = 0xFF;
  frame.src_mac[0] = 0x00;
  frame.src_mac[1] = 0x80;
  frame.src_mac[2] = 0x10;
  frame.src_mac[3] = 0xB3;
  frame.src_mac[4] = 0xEB;
  frame.src_mac[5] = 0xB7;
  frame.etherType = 0x0008;
  tx_bytes(&frame, sizeof(frame));

  ip.versionAndHeaderLength = 0x45;
  ip.typeOfService = 0;
  ip.packetLength = ntohs(sizeof(ip) + sizeof(udp) + numBytesRoundedUpToEven);
  ip.identification = 0;//ntohs(17299); // ???
  ip.flagsAndFragmentOffset = 0;
  ip.timeToLive = 128;
  ip.protocol = 17; // udp
  ip.headerChecksum = 0; // not checksummed
  /*
  ip.src_ip[0] = 192;
  ip.src_ip[1] = 168;
  ip.src_ip[2] = 0;
  ip.src_ip[3] = 182;
  */
  ip.src_ip[0] = 87;
  ip.src_ip[1] = 100;
  ip.src_ip[2] = 208;
  ip.src_ip[3] = 236;

  ip.dst_ip[0] = 255;
  ip.dst_ip[1] = 255;
  ip.dst_ip[2] = 255;
  ip.dst_ip[3] = 255;
  tx_bytes(&ip, sizeof(ip));

  udp.src_port = ntohs(49739);
  udp.dst_port = ntohs(4009);
  udp.length = ntohs(sizeof(udp) + numBytesRoundedUpToEven);
  udp.checksum = 0;
  tx_bytes(&udp, sizeof(udp));

  tx_bytes(data, numBytes);
  if ((numBytes & 1))
  {
    unsigned char zero = 0;
    tx_bytes(&zero, 1);
  }
}

int main()
{
  int success = rrnet_init();
  if (success) printf("RR-NET INITIALIZED\n");
  else printf("RR-NET NOT FOUND!\n");

//  for(;;)
  //  wait_for_packets();

  for(int i = 0; i < 16; ++i)
  {
    const char data[16] = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
    send_udp_broadcast(data, sizeof(data));
  }
  return 0;
}
