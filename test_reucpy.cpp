#include <stdio.h>
#include <reucpy.h>

int main()
{
  int num_banks = reu_init();
  printf("DETECTED %lu BYTES OF REU MEMORY.\n", (unsigned long)num_banks*65536);
  printf("REU STATUS REGISTER: 0X%X.\n", (int)*(volatile uint8_t*)0xDF00);
  printf("REU VERSION: %d.\n", (int)(*(volatile uint8_t*)0xDF00 & 15));

  // reucpy(void *c64_addr, reu_addr_t reu_addr, uint16_t size, uint8_t direction);
  char data[2] = {0};
  reucpy((void*)0, 0, 2, RAM2REU);
  reucpy(data, 0, 2, REU2RAM);
  printf("REU READ FROM CPU ADDR $0000: %X\n", (int)data[0]);
  printf("REU READ FROM CPU ADDR $0001: %X\n", (int)data[1]);
  data[0] = 0x3F;
  data[1] = 0x3F;
  reucpy(data, 0, 2, RAM2REU);
  reucpy((void*)0, 0, 2, REU2RAM);
  unsigned char addr0 = *(volatile unsigned char *)0;
  unsigned char addr1 = *(volatile unsigned char *)1;
  printf("REU WRITE TO CPU ADDR $0000: %X\n", (int)addr0);
  printf("REU WRITE TO CPU ADDR $0001: %X\n", (int)addr1);
}
