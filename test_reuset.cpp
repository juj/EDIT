#include <stdio.h>
#include <reu.h>

#ifdef NDEBUG
#error This needs to be compiled with assertions enabled!
#endif

int main()
{
  int num_banks = reu_init();
  printf("DETECTED %lu BYTES OF REU MEMORY.\n", (unsigned long)num_banks*65536);
  printf("REU STATUS REGISTER: 0X%X.\n", (int)*(volatile uint8_t*)0xDF00);
  printf("REU VERSION: %d.\n", (int)(*(volatile uint8_t*)0xDF00 & 15));

  // reucpy(void *c64_addr, reu_addr_t reu_addr, uint16_t size, uint8_t direction);
  // reuset(reu_addr_t reu_addr, uint8_t val, uint16_t size);
  reuset(100, 0xEE, 1);
  u8 data = 0;
  reucpy(&data, 100, 1, REU2RAM);
  printf("READ VALUE %x FROM REU.\n", data);
  assert(data == 0xEE);

  for(int i = 0; i < 64; ++i)           // Clear first 4MB of REU memory to init
    reuset((u24)i << 16, 0x6A+i, 0);         // the A* closed set.

  for(int i = 0; i < 64; ++i)           // Clear first 4MB of REU memory to init
  {
    u8 data = 0;
    reucpy(&data, ((u24)i << 16) + 100, 1, REU2RAM);
    assert(data == 0x6A+i);
  }
}
