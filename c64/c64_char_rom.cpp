#include <string.h>

static volatile unsigned char *processor_port = (unsigned char *)0x01;
static volatile unsigned char *mem_setup = (unsigned char *)0xD018;
static volatile unsigned char *vic_bank = (unsigned char *)0xDD00;
static volatile unsigned char *screen_ram_starting_address = (unsigned char *)0x0288; // high byte part (*256)
static volatile unsigned char *new_char_ram = (unsigned char *)0xF000;

static const unsigned char forward_brace[8] = 
{
  0b00001110,
  0b00010000,
  0b00010000,
  0b01100000,
  0b00010000,
  0b00010000,
  0b00001110,
  0b00000000,
};

static const unsigned char backward_brace[8] = 
{
  0b01110000,
  0b00001000,
  0b00001000,
  0b00000110,
  0b00001000,
  0b00001000,
  0b01110000,
  0b00000000,
};

static const unsigned char backslash[8] = 
{
  0b00000000,
  0b11000000,
  0b01100000,
  0b00110000,
  0b00011000,
  0b00001100,
  0b00000110,
  0b00000000,
};

static const unsigned char Xor[8] = 
{
  0b00000000,
  0b00011000,
  0b00111100,
  0b01100110,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
};

void setup_cpp_programming_font()
{
  asm volatile("SEI"); // We'll swap KERNAL out, so disable interrupts to avoid execution going there

  unsigned char old_processor_port = *processor_port;
  *processor_port = (old_processor_port & ~7) | 1; // Set mem map to 001b: { RAM, CHARROM, RAM }

  memcpy((void*)new_char_ram, (void*)0xD800, 2048); // Copy CHARROM over to RAM under KERNAL ROM 0xF000
  memcpy((void*)(new_char_ram + 0x6B*8), forward_brace, 8);
  memcpy((void*)(new_char_ram + 0x73*8), backward_brace, 8);
  memcpy((void*)(new_char_ram + 0x6E*8), backslash, 8);
  memcpy((void*)(new_char_ram + 30*8), Xor, 8);

  memcpy((void*)(new_char_ram + (0x6B+0x80)*8), forward_brace, 8);
  memcpy((void*)(new_char_ram + (0x73+0x80)*8), backward_brace, 8);
  memcpy((void*)(new_char_ram + (0x6E + 0x80)*8), backslash, 8);
  memcpy((void*)(new_char_ram + (30+0x80)*8), Xor, 8);

  for(int i = 0; i < 8; ++i)
  {
    new_char_ram[(0x6B+0x80)*8+i] = ~new_char_ram[(0x6B+0x80)*8+i];
    new_char_ram[(0x73+0x80)*8+i] = ~new_char_ram[(0x73+0x80)*8+i];
    new_char_ram[(0x6E + 0x80)*8+i] = ~new_char_ram[(0x6E + 0x80)*8+i];
    new_char_ram[(30+0x80)*8+i] = ~new_char_ram[(30+0x80)*8+i];
  }

  *processor_port = old_processor_port; // Restore memory map back to original state

  *vic_bank &= 0xFC; // Set VIC base to 0xC000
  *mem_setup = 0x3C; // Set Font RAM offset to +0x3000 and Screen RAM offset to +0x0800
  *screen_ram_starting_address = 0xCC; // Fix CHROUT KERNAL call to print at 0xCC00.

  asm volatile("CLI");
}
