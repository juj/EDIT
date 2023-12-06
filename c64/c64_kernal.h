#pragma once

// C64 KERNAL ROM functions

#include <stdint.h>
#include "mystdio.h"

#ifdef __C64__

// TODO: Might want to use this form, but can't due to https://github.com/llvm-mos/llvm-mos/issues/392
// #define _ASM __attribute__((leaf)) __asm__
// #define _KERNAL static __inline__ __attribute__((__always_inline__, __nodebug__, leaf))
#define _ASM __asm__
#define _KERNAL static __inline__ __attribute__((__always_inline__, __nodebug__))
#define _HI(x) ((uint8_t)((uint16_t)(x)>>8))
#define _LO(x) ((uint8_t)(uint16_t)(x))
#define _U16(lo, hi) (((uint16_t)(hi) << 8) | (lo))
// Inline GCC asm syntax has a weird way of requiring different syntax to be used to declare
// clobbered registers, depending on whether those registers are, or are not used as inputs.
// Use the following macros to better declare intent in the output blocks that a specific
// output directive is used to denote a clobber rather than a real output.
// See https://github.com/llvm-mos/llvm-mos/issues/385
#define CLOB_A "=a"(a)
#define CLOB_X "=x"(x)
#define CLOB_Y "=y"(y)

_KERNAL uint8_t __stack_avail()
{
  uint8_t stack_pointer;
  _ASM("TSX":"=x"(stack_pointer)::);
  return stack_pointer;
}
#define NEED_STACK(bytes) assert(__stack_avail() > bytes)

// Initialize screen editor & 6567 video chip
_KERNAL void _CINT()   { NEED_STACK(4); _ASM volatile("JSR $FF81":::"a","x","y","memory"); }

// Initialize I/O devices
_KERNAL void _IOINIT() { _ASM volatile("JSR $FF84":::"a","x","y"); }

// Perform RAM test
_KERNAL void _RAMTAS() { NEED_STACK(2); _ASM volatile("JSR $FF87":::"a","x","y","memory"); }

// Restore default system and interrupt vectors
_KERNAL void _RESTOR() { NEED_STACK(2); _ASM volatile("JSR $FF8A":::"a","x","y"); }

// Read/Set KERNAL indirect vector table
_KERNAL void _VECTOR_READ(uintptr_t vec[16])      { NEED_STACK(2); assert(vec); uint8_t x,y; _ASM("SEC\nJSR $FF8D":CLOB_X,CLOB_Y:"x"(_LO(vec)),"y"(_HI(vec)):"a","c","memory"); }
_KERNAL void _VECTOR_SET(const uintptr_t vec[16]) { NEED_STACK(2); assert(vec); uint8_t x,y; _ASM volatile("CLC\nJSR $FF8D":CLOB_X,CLOB_Y:"x"(_LO(vec)),"y"(_HI(vec)):"a","c"); }

// Enable or mute printing of system error and control messages. If bit 7 is set in flags, error messages are enabled. If bit 6 is set in flags, control messages are enabled.
_KERNAL void _SETMSG(uint8_t flags) { NEED_STACK(2); _ASM volatile("JSR $FF90"::"a"(flags):); }

// Send a Secondary Address to a Device on the Serial Bus after LISTEN
_KERNAL void _SECOND(uint8_t addr)  { NEED_STACK(8); uint8_t a; _ASM volatile("JSR $FF93":CLOB_A:"a"(addr):); }

// Send a Secondary Address to a Device on the Serial Bus after TALK
_KERNAL void _TKSA(uint8_t addr)    { NEED_STACK(8); uint8_t a; _ASM volatile("JSR $FF96":CLOB_A:"a"(addr):); }

// Read/Set Top of RAM Pointer
_KERNAL uint16_t _MEMTOP_READ() { NEED_STACK(2); uint8_t lo, hi; _ASM("SEC\nJSR $FF99":"=x"(lo),"=y"(hi)::"c"); return _U16(lo, hi); }
_KERNAL void _MEMTOP_SET(uint16_t membot) { NEED_STACK(2); uint8_t x,y; _ASM volatile("CLC\nJSR $FF99":CLOB_X,CLOB_Y:"x"(_LO(membot)),"y"(_HI(membot)):"c"); }

// Read/Set Bottom of RAM Pointer
_KERNAL uint16_t _MEMBOT_READ() { uint8_t lo, hi; _ASM("SEC\nJSR $FF9C":"=x"(lo),"=y"(hi)::"c"); return _U16(lo, hi); }
_KERNAL void _MEMBOT_SET(uint16_t membot) { uint8_t x,y; _ASM volatile("CLC\nJSR $FF9C":CLOB_X,CLOB_Y:"x"(_LO(membot)),"y"(_HI(membot)):"c"); }

// Scans the keyboard matrix.
// Normally this is done automatically by the C64 KERNAL default IRQ handler so this function does
// not need to be manually called. However if you implement your own IRQ ISR, you may want to call this
// as part of that interrupt service routine.
_KERNAL void _SCNKEY() { NEED_STACK(5); _ASM volatile("JSR $FF9F":::"a","x","y"); }

// Set time-out flag on serial bus
_KERNAL void _SETTMO(uint8_t flag) { NEED_STACK(2); _ASM volatile("JSR $FFA2"::"a"(flag):); }

// Input byte from serial port
_KERNAL uint8_t _ACPTR() { NEED_STACK(13); uint8_t byte; _ASM volatile("JSR $FFA5":"=a"(byte)::"x"); return byte; }

// Output byte to serial port
_KERNAL void _CIOUT(uint8_t byte) { NEED_STACK(5); uint8_t a; _ASM volatile("JSR $FFA8":CLOB_A:"a"(byte):); }

// Send UNTALK command to serial bus
_KERNAL void _UNTLK() { NEED_STACK(8); _ASM volatile("JSR $FFAB":::"a"); }

// Send UNLISTEN command to serial bus
_KERNAL void _UNLSN() { NEED_STACK(8); _ASM volatile("JSR $FFAE":::"a"); }

// Send LISTEN command to serial bus
_KERNAL void _LISTEN(uint8_t device) { uint8_t a; _ASM volatile("JSR $FFB1":CLOB_A:"a"(device):); }

// Send TALK command to serial bus
_KERNAL void _TALK(uint8_t device)   { NEED_STACK(8); uint8_t a; _ASM volatile("JSR $FFB4":CLOB_A:"a"(device):); }

// Return I/O status byte
_KERNAL uint8_t _READST() { NEED_STACK(2); uint8_t status; _ASM volatile("JSR $FFB7":"=a"(status)::); return status; }

// Set logical file number, device number, secondary address for I/O.
// Logical file number: A unique number to associate with the file, in range 1-127.
// Meaning of Secondary Address from C64 1530 Datasette manual for cassette tape operation:
// - 0: Read from tape. >0: Write to tape.
// - bit 0: If set, denote a nonrelocatable program. If clear, denote a relocatable program.
// - bit 1: If set, write an End-Of-Tape marker after the file to denote the end of all data on the tape.
// Meaning of Secondary Address for disks(?):
// If secondary address is 0: default file type is PRG, and file is opened for reading. "Relocating" load is performed.
// If secondary address is 1: default file type is PRG, and file is opened for writing. Nonrelocating load is performed.
// If secondary address >= 2: use the information present in SETNAM in format ",S,R" or ",P,W" etc. to determine file type and read/write
// When opening a MPS-803 printer:
// - if secondary address is 7: print business PETSCII
// - if secondary address is 0: print graphics PETSCII
_KERNAL void _SETLFS(uint8_t file_no, uint8_t device_address, uint8_t secondary_address) { NEED_STACK(2); _ASM volatile("JSR $FFBA"::"a"(file_no),"x"(device_address),"y"(secondary_address):); }

// Sets filename for OPEN command
_KERNAL void _SETNAM(const char *filename, uint8_t length) { NEED_STACK(2); assert(filename || !length); _ASM volatile("JSR $FFBD"::"a"(length), "x"(_LO(filename)), "y"(_HI(filename)):); }

// Open a Logical File. Returns error code (0=no error).
// C64 supports up to 10 simultaneously loaded files.
_KERNAL uint8_t _OPEN() { uint8_t error; _ASM volatile("JSR $FFC0\nBCS error_open%=\nLDA #0\nerror_open%=:":"=a"(error)::"x","y","c"); return error; }

// Close a Logical File. Returns error code (0=no error).
_KERNAL uint8_t _CLOSE(uint8_t file_no) { NEED_STACK(2/*todo:max unknown*/); uint8_t error; _ASM volatile("JSR $FFC3":"=a"(error):"a"(file_no):"x","y","c"); return error; }

// Designate a Logical File as the current Input Channel. Returns error code (0=no error).
_KERNAL uint8_t _CHKIN(uint8_t file_no)
{
  uint8_t error,x;
  _ASM volatile(
    "JSR $FFC6\n"
    "BCS end%=\n"
    "LDA #0\n" // No error occurred, so clear A register that would contain the error
    "end%=:\n"
    :"=a"(error),CLOB_X:"x"(file_no):"y","c");
  return error;
}

// Designate a Logical File As the current Output Channel. Returns error code (0=no error).
_KERNAL uint8_t _CHKOUT(uint8_t file_no)
{
  NEED_STACK(4/*todo:max unknown*/);
  uint8_t error,x;
  _ASM volatile(
    "JSR $FFC9\n"
    "BCS end%=\n"
    "LDA #0\n"
    "end%=:\n"
    :"=a"(error),CLOB_X:"x"(file_no):"y","c");
  return error;
}

// Restore current Input and Output Devices to the Default Devices
_KERNAL void _CLRCHN() { NEED_STACK(9); _ASM volatile("JSR $FFCC":::"a","x"); }

// Input a character from the current Input Channel
_KERNAL uint8_t _CHRIN() { NEED_STACK(7/*todo:max unknown*/); uint8_t byte;  _ASM volatile("JSR $FFCF":"=a"(byte)::"c"); return byte; }
_KERNAL uint8_t _CHRIN_GET_ERROR(bool *error)
{
  assert(error);
  NEED_STACK(7/*todo:max unknown*/);
  uint8_t byte;
  bool err;
  _ASM volatile(
    "JSR $FFCF\n"
    "LDX #0\n"
    "BCC end%=\n"
    "LDX #1\n"
    "end%=:\n"
    :"=a"(byte), "=x"(err)::"c");
  *error = err;
  if (err) return 0;
  return byte;
}

// Output a character to the current Output Channel.
// Note: calling this function temporarily disables interrupts,
// so will stall the advance of the RTC clock (_RDTIM() below).
_KERNAL void _CHROUT(uint8_t byte) { NEED_STACK(8/*todo:max unknown*/); _ASM volatile("JSR $FFD2"::"a"(byte):"c"); }
_KERNAL bool _CHROUT_GET_ERROR(uint8_t byte)
{
  NEED_STACK(8/*todo:max unknown*/);
  bool error;
  _ASM volatile(
    "JSR $FFD2\n"
    "LDA #0\n"
    "ROL\n"
    :"=a"(error):"a"(byte):"c");
  return error;
}

// Load file to RAM, or verify (compare) file contents against RAM. To use, call SETNAM+SETLFS+LOAD. This function
// does not open a file handle, so no call to CLOSE is needed afterwards.
// Note: Only files stored with the SAVE function may be loaded in this way. Files stored with the character-based
// OPEN+CHKOUT+CHROUT+CLOSE method can not be loaded with this function.
_KERNAL uint8_t _LOAD(uint8_t verify, void *dst, void **dst_end)
{
  assert(dst);
  uint8_t error, lo, hi;
  _ASM volatile("JSR $FFD5":"=a"(error), "=x"(lo), "=y"(hi):"a"(verify),"x"(_LO(dst)),"y"(_HI(dst)):"c","memory");
  if (dst_end) *dst_end = (void*)_U16(lo, hi);
  return error;
}

// Save contiguous section [start, end[ from memory to a file specified by previous call to SETNAM+SETLFS.
// This function does not open a file handle, so no call to CLOSE is needed afterwards.
// Note: When a file is written this way, it is only possible to load it using the LOAD function afterwards.
// The character-based OPEN+CHKIN+CHRIN+CLOSE method cannot be used to load a file that was written with SAVE.
_KERNAL uint8_t _SAVE(const void *start, const void *end)
{
  uint8_t error,x,y;
  // JSR $FFD8: KERNAL ROM 'SAVE' routine
  // Writes a contiguous region of bytes from memory at [start, end[ to disk.
  // Inputs:
  //   A: a pointer to a 16-bit variable in the zero-page memory address that contains the value of 'start'
  //   X: low 8 bits of 'end' address
  //   Y: high 8 bits of 'end' address
  // Outputs:
  //   C: If 1, error occurred. If 0, no error.
  //   A: If error occurred, contains error code. If no error occurred, contains garbage
  // Clobbers: X, Y
  _ASM volatile(
    "LDA #%[zp]\n"
    "JSR $FFD8\n" // Call the 'SAVE' KERNAL ROM routine
    "BCS error_save%=\n"
    "LDA #0\n"
    "error_save%=:\n"
    :"=a"(error),CLOB_X,CLOB_Y:[zp]"r"(start),"x"(_LO(end)),"y"(_HI(end)):"c");
  return error;
}

// Set the current real-time clock time ("Jiffies" counter)
typedef unsigned _BitInt(24) _Jiffies;
_KERNAL void _SETTIM(_Jiffies jiffies) { NEED_STACK(2/*todo:max unknown*/); _ASM volatile("JSR $FFDB"::"a"((uint8_t)(jiffies>>16)),"x"((uint8_t)(jiffies>>8)),"y"((uint8_t)(jiffies))); }

// Read the RTC "Jiffies" clock. This clock ticks at fixed 60Hz,
// even on PAL systems. Note: The RTC clock does not advance
// while interrupts are disabled.
_KERNAL _Jiffies _RDTIM()
{
  NEED_STACK(2/*todo:max unknown*/);
  uint8_t lo, mid, hi;
  // TODO: Odd, it seems I need to extract these in opposite
  // order than is documented (docs say A is MSB and Y is LSB
  // but in practice I find the opposite direction must be used)
  _ASM volatile("JSR $FFDE":"=y"(hi),"=x"(mid),"=a"(lo)::);
  return ((_Jiffies)hi << 16) | ((_Jiffies)mid << 8) | lo;
}

// Read the two lowest bytes of RTC clock only
_KERNAL uint16_t _RDTIM16()
{
  NEED_STACK(2/*todo:max unknown*/);
  uint8_t lo, mid;
  // TODO: Odd, it seems I need to extract these in opposite
  // order than is documented (docs say A is MSB and Y is LSB
  // but in practice I find the opposite direction must be used)
  _ASM volatile("JSR $FFDE":"=x"(mid),"=a"(lo)::"y");
  return ((uint16_t)mid << 8) | lo;
}

// Returns 50 on PAL systems and 60 on NTSC systems.
_KERNAL uint8_t _FRAMES_PER_SEC()
{
  uint8_t frames_per_sec;
  _ASM(
    "    SEI\n"
    "l1%=: LDA $D012\n"
    "l2%=: CMP $D012\n"
    "    BEQ l2%=\n"
    "    BMI l1%=\n"
    "    CMP #$20\n"
    "    BCC ntsc%=\n"
    "    LDA #50\n"
    "    JMP done%=\n"
    "ntsc%=: LDA #60\n"
    "done%=: CLI\n":"+a"(frames_per_sec)::"c");
  return frames_per_sec;
}

// Query Stop key indicator. If pressed, call CLRCHN and clear keyboard buffer.
_KERNAL uint8_t _STOP()  { uint8_t stop; _ASM volatile("JSR $FFE1\nTSX\nTXA\nAND #2":"=a"(stop)::"x"); return stop; }

// Get a character from current Input Channel (compare to _CHRIN, which does not pause to wait, but returns EOF if buffer is empty)
// TODO: Hmmno, _GETIN() won't block to wait either
// Returns PETSCII code of character.
_KERNAL uint8_t _GETIN() { NEED_STACK(7/*todo:max unknown*/); uint8_t byte; _ASM volatile("JSR $FFE4":"=a"(byte)::"x","y","c"); return byte; }

// Close All Logical I/O Files. (does not actually close, but discards track of all open files, as if they were closed)
_KERNAL void _CLALL()    { NEED_STACK(11); _ASM volatile("JSR $FFE7":::"a","x"); }

// Update the RTC Clock and Check for the STOP Key (called regularly by KERNAL)
_KERNAL void _UDTIM()    { NEED_STACK(2);  _ASM volatile("JSR $FFEA":::"a","x"); }

// Get number of screen rows/columns
_KERNAL void _SCREEN(uint8_t *width, uint8_t *height) { NEED_STACK(2); _ASM("JSR $FFED":"=x"(*width),"=y"(*height)::"a","memory"); }

// Read/Set current text cursor position
_KERNAL void _PLOT_READ(uint8_t *x, uint8_t *y)       { NEED_STACK(2); _ASM("SEC\nJSR $FFF0":"=x"(*y),"=y"(*x)::"c","memory"); }
_KERNAL void _PLOT_SET(uint8_t x, uint8_t y)          { NEED_STACK(2); _ASM volatile("CLC\nJSR $FFF0"::"x"(y),"y"(x):"a","c"); }

// Fetch CIA #1 base address
_KERNAL uintptr_t _IOBASE() { NEED_STACK(2); uint8_t lo, hi; _ASM("JSR $FFF3":"=x"(lo),"=y"(hi)::); return _U16(lo, hi); }

/////////////////////////////////////////////
// Custom extensions to KERNAL functionality:

// Soft-resets the system.
// TODO: This won't work on C16, C116, Plus/4 or C128x: https://www.c64-wiki.com/wiki/Reset_(Process)
_KERNAL void _Noreturn _RESET() { _ASM volatile("JMP ($FFFC)"); __builtin_unreachable(); }

// Returns true if there is a key pending in the keyboard input queue.
_KERNAL bool _KBHIT() { return *(volatile unsigned char *)0x00C6; }

// Clears the screen (resets both character and color data) and moves cursor to top-left (0,0).
_KERNAL void _CLRSCR() { _ASM volatile("JSR $E544":::"a","x","y","c"); }

// Disables interrupts. Be sure to re-enable interrupts after executing a critical block.
// Note that these calls do not have a nesting counter, i.e. calls to _DISABLE_INTR()
// won't "stack up". After multiple calls to _DISABLE_INTR(), interrupts can be re-enabled
// with just one call to _ENABLE_INTR().
_KERNAL void _DISABLE_INTR() { _ASM volatile("SEI"); }

// Re-enables interrupts.
_KERNAL void _ENABLE_INTR() { _ASM volatile("CLI"); }

// Sets the screen border color (lowest 4 bits of color, 0-15)
_KERNAL void _BORDER(uint8_t color) { *(volatile uint8_t*)0xD020 = color; }

// Synchronously pauses for the given number of Jiffies.
_KERNAL void _SLEEP(uint16_t jiffies)
{
  _Jiffies target = _RDTIM() + jiffies;
  while(_RDTIM() < target) /*nop*/;
}

// Returns true if any of the Datassette buttons is down (Record/Play/FF/Rewind)
_KERNAL bool _DATASSETTE_BUTTON_DOWN() { return ((*(volatile uint8_t*)0x01) & 0x10) == 0; }

// Use this function to effectively halt execution in a fashion that the compiler
// will not be able to understand to optimize away.
_KERNAL void _HALT() { _ASM volatile("1: JMP 1b"); }

#endif // ~__C64__
