#include <string.h>
#include <stdio.h>
#include <stdint.h>

static volatile uint8_t * const rrnet_ctl      = (uint8_t *)0xDE01;
static volatile uint8_t * const cs_packet_page = (uint8_t *)0xDE02;
static volatile uint8_t * const cs_packet_data = (uint8_t *)0xDE04;

extern "C" void receive_and_run_prg();

extern char __restore_overlay_start, __restore_overlay_dst, __restore_overlay_dst_end;

static void write_u16_lo_hi(volatile uint8_t *dst, uint16_t value)
{
  dst[0] = (uint8_t)value;
  dst[1] = (uint8_t)(value>>8);
}

static void cs_set_page(uint16_t page) { write_u16_lo_hi(cs_packet_page, page); }

static void cs_write_page_data(uint16_t page, uint16_t data)
{
  cs_set_page(page);
  write_u16_lo_hi(cs_packet_data, data);
}

int main()
{
  *rrnet_ctl |= 1; // enable clock port
  cs_set_page(0);
  cs_write_page_data(0x0114/*pp_self_ctl*/, 0x0055); // reset chip
  cs_write_page_data(0x0104/*pp_rx_ctl*/, 0x0D05); // accept individual and broadcast packets
  cs_write_page_data(0x0112/*pp_line_ctl*/, 0xD3); // Enable RX and TX

  /*
  char *s = &__restore_overlay_start;
  for(char *d = &__restore_overlay_dst; d != &__restore_overlay_dst_end;)
    *d++ = *s++;
    */
/*
printf("START: 0X%X\n", (unsigned int)&__restore_overlay_start);
printf("DST: 0X%X\n", (unsigned int)&__restore_overlay_dst);
printf("DST END: 0X%X\n", (unsigned int)&__restore_overlay_dst_end);
*/
  int i = 0;
  for(;;)
  {
    asm volatile("JSR $E544");
    printf("PRG: %d\n", i);
    ++i;
    receive_and_run_prg();
  }
}
