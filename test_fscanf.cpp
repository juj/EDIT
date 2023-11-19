#include <stdio.h>

int main()
{
  FILE *f = fopen("test.txt", "w");
  fputs("test 123", f);
  fclose(f);
  f = fopen("test.txt", "r");
  int num = 1999;

  int n = fscanf(f, "%d", &num);
  printf("n=%d, num=%d\n", n, num);

  char str[8];
  n = fscanf(f, "%s", &str);
  printf("n=%d, str=%s\n", n, str);

  n = fscanf(f, "%d", &num);
  printf("n=%d, num=%d\n", n, num);
  fclose(f);
}
