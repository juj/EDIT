int main()
{
  asm volatile("JSR $CF00":::"a");
  return 0;
}
