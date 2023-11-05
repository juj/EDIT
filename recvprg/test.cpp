#include <string.h>
#include <stdio.h>

extern "C" void receive_and_run_prg();

extern char __restore_overlay_start, __restore_overlay_dst, __restore_overlay_dst_end;

int main()
{
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
  receive_and_run_prg();
}
