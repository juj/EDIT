#include <string.h>

extern "C" void recv_prg();

int main()
{
/*
  volatile unsigned char *d = (unsigned char *)0xCC00;
  volatile unsigned char *s = (unsigned char *)recv_prg;
  for(int i = 0; i < 1024; ++i)
    *d++ = *s++;
//  memcpy((volatile void*)0xCC00, (void*)recv_prg, 1024);
  asm volatile("JMP $CC00");
*/
  recv_prg();
}
