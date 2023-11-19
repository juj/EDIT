#include "c64/mystdio.h"
#include "c64/c64_kernal.h"

int main()
{
  unsigned char keyCode = 0, ch;
  while(!keyCode)
  {
    keyCode = _GETIN();
  }

  FILE *f = fopen("TEST.TXT", "w");
  fputs("TEST1\n", f);
  fputs("TE", f);
  fputs("ST2\nTES", f);
  fputs("T3", f);
  fclose(f);

  f = fopen("TEST.TXT", "r");
  char str1[8] = { 'A', 'A', 'A', 'A', 'A', 'A', 'A' };
  char str2[8] = { 'B', 'B', 'B', 'B', 'B', 'B', 'B' };
  char str3[8] = { 'C', 'C', 'C', 'C', 'C', 'C', 'C' };
  char str4[8] = { 'D', 'D', 'D', 'D', 'D', 'D', 'D' };
  char *s1 = fgets(str1, sizeof(str1), f);
  char *s2 = fgets(str2, sizeof(str2), f);
  char *s3 = fgets(str3, sizeof(str3), f);
  char *s4 = fgets(str4, sizeof(str4), f);
  fclose(f);

  printf("STR1: \"%s\"\n", str1);
  printf("STR2: \"%s\"\n", str2);
  printf("STR3: \"%s\"\n", str3);
  printf("STR4: \"%s\"\n", str4);

  printf("STR1: %p, S1: %p\n", str1, s1);
  printf("STR2: %p, S2: %p\n", str2, s2);
  printf("STR3: %p, S3: %p\n", str3, s3);
  printf("STR4: %p, S4: %p\n", str4, s4);
}
