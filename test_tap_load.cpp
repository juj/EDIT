#include <stdio.h>
#include "c64_kernal.h"
#include <stdio.h>

//  char data[512];

int main()
{
  char str[80];

  printf("TEST WRITING TO TAPE, FILE EX1.TXT...\n");
  FILE *handle = fopen("EX1.TXT", "1w");
  for(int i = 0; i < 10; ++i)
  {
    char str[80];
    sprintf(str, "%d\n", i);
    fputs(str, handle);
  }
  fclose(handle);
  printf("WRITE DONE!\n");
  printf("PLEASE REWIND AND PRESS ANY KEY.\n");
  char key = 0;
  while (!key) key = _GETIN();

  handle = fopen("", "1r");

  while(fgets(str, sizeof(str), handle))
    printf("%s", str);
  fclose(handle);


/*
  memset(data, 0, sizeof(data));
  int dpos = 0;
  for(;;)
  {
    uint8_t val = _CHKIN(handle->handle);
    if (val) break;
    uint8_t err = _READST();
    if (err) break;
    bool e;
    uint8_t ch = _CHRIN_GET_ERROR(&e);
    if (e) break;
    data[dpos++] = ch;
    err = _READST();
    if (err) break;
    _CLRCHN();
    _CHROUT(ch == '\n' ? 'Z' : ch);
  }
  _CLOSE(handle->handle);
  _CLRCHN();
  printf("%s\n", data);
*/
  printf("TEST FINISHED!\n");
  for(;;);
}
