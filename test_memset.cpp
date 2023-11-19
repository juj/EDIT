#include <stdio.h>

int main()
{
  char *data = (char*)malloc(520);
  data[0] = 0xEE;
  data[519] = 0xFF;
  uint8_t fill_value = 1;
  for(int i = 0; i < 518; ++i)
  {
    if ((i&15) == 0) printf("%d\n", i);
    __memset(data+1, ++fill_value, i);
    assert(data[0] == 0xEE);
    assert(data[519] == 0xFF);
    for(int j = 0; j < i; ++j)
      assert(data[1+j] == fill_value);
  }
  printf("ALL DONE!\n");
}
