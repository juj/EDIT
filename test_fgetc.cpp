#include "c64/mystdio.h"
#include "c64/c64_kernal.h"

int main()
{
  unsigned char keyCode = 0;
  while(!keyCode) keyCode = _GETIN();

  FILE *f = fopen("TEST.TXT", "w");
  fputs("A1\n", f);
  fputs("B", f);
  fputs("2\nC", f);
  fputs("3", f);
  fclose(f);

  f = fopen("TEST.TXT", "r");
  int ch = 0;
  while(ch != EOF)
  {
    ch = fgetc(f);
    printf("%d %c\n", ch, (uint8_t)ch);
  }
  fclose(f);
}
