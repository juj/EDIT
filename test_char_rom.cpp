#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile unsigned char *processor_port = (unsigned char *)0x01;
static volatile unsigned char *mem_setup = (unsigned char *)0xD018;
static volatile unsigned char *vic_bank = (unsigned char *)0xDD00;
static volatile unsigned char *screen_ram = (unsigned char *)0xCC00;
static volatile unsigned char *screen_ram_starting_address = (unsigned char *)0x0288; // high byte part (*256)
static volatile unsigned char *new_char_ram = (unsigned char *)0xF000;

int main()
{
#if 1
//  unsigned char *tmp = (unsigned char *)malloc(2048);

  asm volatile("SEI"); // We'll swap KERNAL out, so disable interrupts to avoid execution going there

  unsigned char old_processor_port = *processor_port;
  *processor_port = (old_processor_port & ~7) | 1; // Set mem map to 001b: { RAM, CHARROM, RAM }

  memcpy((void*)new_char_ram, (void*)0xD000, 2048); // Copy CHARROM over to RAM under KERNAL ROM 0xF000
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

//  free(tmp);

//  for(int i = 32; i < 128; ++i)
//    asm volatile("JSR $FFD2"::"a"((unsigned char)i):"c");
  for(unsigned int i = 0; i < 1024; ++i)
    screen_ram[i] = i < 256 ? i : i == 9*40+5 ? 0x6B : i == 9*40+7 ? 0x73 : i == 11*40+5 ? 0x6E : i == 11*40+7 ? '/' 
      : i == 9*40+9 ? 0x3E
      : i == 8*40+7 ? 0x3E
  : 32;


#if 1
  // Bits 0-2 control CPU port map for BASIC/KERNAL/FONT/IO
  printf("MEMORY MAP: %d\n", ((unsigned int)*processor_port) & 7);

  // Bits 0-1 of VIC register select the VIC bank
  unsigned int vic_idx = (*vic_bank & 3);
  unsigned int vic_base = 0xC000-vic_idx*0x4000;
  printf("VIC BANK: %d AT $%04X\n", vic_idx, vic_base);

  // Bits 1-3 of Memory setup register choose the character map offset
  /*
  int char_rom_idx = (*mem_setup >> 1) & 7;
  unsigned int char_rom_ofs = char_rom_idx*0x800;
  printf("CHAR ROM OFFSET: %d AT $%04X\n", char_rom_idx, char_rom_ofs);

  // Calculate the effective memory address where VIC reads the font from:
  printf("EFFECTIVE CHAR ROM ADDRESS: $%04X\n", vic_base + char_rom_ofs);
  */
  // Bits 4-7 specify screen ram offset
  int screen_ram_idx = *mem_setup >> 4;
  unsigned int screen_ram_ofs = screen_ram_idx*0x0400;
  unsigned int screen_ram_ptr = vic_base + screen_ram_ofs;
  printf("SCREEN RAM OFFSET: %d AT $%04X\n", screen_ram_idx, screen_ram_ofs);
  printf("EFFECTIVE SCREEN RAM ADDRESS: $%04X\n", screen_ram_ptr);
#endif
  unsigned char ch = 'H';
//  *mem_setup = (*mem_setup & 0x0F) | 0x80;
  asm volatile("JSR $FFD2"::"a"(ch):"c");




  for(;;); // Don't return back, llvm-mos is unable to gracefully return to BASIC.
#endif
}
