void _PLOT_READ(unsigned char *x, unsigned char *y) { asm volatile("SEC\nJSR $FFF0":"=x"(*y),"=y"(*x)::"c","memory"); }
void _PLOT_SET(unsigned char x, unsigned char y)    { asm volatile("CLC\nJSR $FFF0"::"x"(y),"y"(x):"a","c"); }
unsigned char _GETIN() { unsigned char byte; asm volatile("JSR $FFE4":"=a"(byte)::"x","y","c"); return byte; }
void _DISABLE_INTR() { asm volatile("SEI"); }
void _ENABLE_INTR() { asm volatile("CLI"); }

static volatile unsigned char * const cursor_blink = (unsigned char *)0xCC;
static volatile unsigned char * const cursor_blink_delay = (unsigned char *)0xCD;
static volatile unsigned char * const cursor_blink_phase = (unsigned char *)0xCF;
static volatile unsigned char * const color_ram = (unsigned char*)0xD800;
static volatile unsigned char * const screen_ram = (unsigned char*)0x0400;
static volatile unsigned char * const char_under_cursor = (unsigned char *)0xCE;
static volatile unsigned char * const color_under_cursor = (unsigned char*)0x0287;

unsigned char get_key()
{
  for(unsigned char k = 0;;)
    {
      screen_ram[0] = *char_under_cursor;
      color_ram[0] = *color_under_cursor;
    if ((k = _GETIN())) return k;
  }
}

int main()
{
  *cursor_blink = 0; // Enable blink
  for(;;)
  {
    bool paint = false;
    unsigned char key = get_key(), x, y;
    _PLOT_READ(&x, &y);
    _DISABLE_INTR();
    if (*cursor_blink_phase) // If cursor reverse was on, undo the blink
    {
//      *cursor_blink_phase = 0; // Hide cursor
      //*cursor_blink = 1; // Disable cursor
      screen_ram[y*40+x] = *char_under_cursor; // Restore old character
      color_ram[y*40+x] = *color_under_cursor; // and color under cursor
    }
    if (key == 0x11) ++y;
    else if (key == 0x1D) ++x;
    else if (key == 0x91) --y;
    else if (key == 0x9D) --x;
    else
       paint = true;
     if (!paint)
     {
      _PLOT_SET(x, y);
//    *cursor_blink = 0; // Enable cursor again
      *cursor_blink_phase = 1; // Make reverse character visible
      *cursor_blink_delay = 20; // And restore time until next blink
      *char_under_cursor = screen_ram[y*40+x];
      *color_under_cursor = color_ram[y*40+x];
      screen_ram[y*40+x] |= 0x80; // Inverse char at cursor to make it visible
    }
    _ENABLE_INTR();

    if (paint)
    {
      screen_ram[y*40+x] = key;
      color_ram[y*40+x] = color_ram[y*40+x]+1;
    }
  }
}
