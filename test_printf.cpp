#include "c64/mystdio.h"

int main()
{
  Printf("\n");
  Printf("STR UPPERCASE: \"HELLO WORLD!\"\n");
  Printf("STR LOWERCASE: \"hello world!\"\n");
  Printf("\n");
  Printf("INT: %d == -32768\n", -32768);
  Printf("UINT: %u ==  65535\n", 65535);
  Printf("STR: \"%s\" == \"TEST\"\n", "TEST");
  Printf("CHR:    '%c' == 'A'\n", 'A');
  Printf("PERCENT SIGN: %%\n");
  Printf("LOWERCASE HEX: 0x%x == 0xabcd\n", 0xABCD);
  Printf("UPPERCASE HEX: 0x%X == 0xABCD\n", 0xABCD);
}
