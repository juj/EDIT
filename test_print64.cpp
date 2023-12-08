#include <libc.h>

int main()
{
  print64(12345678901234567890ull);
  printf("\n");
  print64(12345678909876543210ull);
  printf("\n");
}
