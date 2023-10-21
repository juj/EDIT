#include "../c64/c64_kernal.h"

static volatile unsigned char * const key_address = (unsigned char *)0x00CB;
static volatile unsigned char * const key_modifiers = (unsigned char *)0x028D;

int main()
{
  printf("PRESS SOME KEYS:\n");

  for(;;)
  {
    unsigned char keyCode = 0;
    while(!keyCode)
      keyCode = _GETIN();

    unsigned char modifiers = *key_modifiers;
    bool is_shift = (modifiers&1);
    bool is_cmdre = (modifiers&2);
    bool is_ctrl = (modifiers&4);
    unsigned char key_matrix_code = *key_address;

    printf("KEY:%02XH MAT:%02XH '%c' %s %s %s\n",
      (int)keyCode, (int)key_matrix_code,
      keyCode,
      is_shift ? "SHFT" : "    ",
      is_cmdre ? "CMDR" : "    ",
      is_ctrl ? "CTRL" : "    ");
  }
}
