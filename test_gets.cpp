#include <stdio.h>
#include <limits.h>

int main()
{
  *(volatile uint8_t *)0x286 = 4;
  printf("INPUT? ");
  char fn[PATH_MAX];
  char *s = gets_s(fn, sizeof(fn));
  printf("YOU ENTERED \"%s\"\n", s);
}
