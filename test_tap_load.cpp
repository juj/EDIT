#include <stdio.h>
#include "c64_kernal.h"
#include <stdio.h>

int main()
{
  char str[80];

  printf("TEST WRITING TO TAPE, FILE EX1.TXT...\n");
  FILE *handle = fopen("EX1.TXT", "1w");
#if 1
  for(int i = 0; i < 23; ++i)
  {
    char str[80];
    sprintf(str, "%d THIS IS A 40 COLUMN LINE ABCDEFGH %d\n", i, i);
    fputs(str, handle);
    _SLEEP(240);
  }
#else
  char test[] = "int main(){}";
  fwrite8(test, 12, handle);
  fputc('\n', handle);
#endif
  fclose(handle);
  printf("WRITE DONE. PLEASE STOP AND REWIND.\n");
  while(_DATASSETTE_BUTTON_DOWN());

  // Wait for the rewind
  while(!_DATASSETTE_BUTTON_DOWN());
  while(_DATASSETTE_BUTTON_DOWN());

  handle = fopen("", "1r");
  while(fgets(str, sizeof(str), handle))
  {
    _SLEEP(240);
    printf("%s", str);
  }
  fclose(handle);
  printf("TEST FINISHED!\n");
  for(;;); // Don't return to BASIC, so that the test results text remain on screen
}
