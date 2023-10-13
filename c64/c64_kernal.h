#pragma once

// C64 KERNAL ROM functions

#include <stdint.h>

#ifdef __C64__

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

// Initialize screen editor & 6567 video chip
_KERNAL void _CINT()   { __asm__ volatile("JSR $FF81":::"a","x","y","memory"); }

// Initialize I/O devices
_KERNAL void _IOINIT() { __asm__ volatile("JSR $FF84":::"a","x","y"); }

// Perform RAM test
_KERNAL void _RAMTAS() { __asm__ volatile("JSR $FF87":::"a","x","y","memory"); }

// Restore default system and interrupt vectors
_KERNAL void _RESTOR() { __asm__ volatile("JSR $FF8A":::"a","x","y"); }

// Read/Set KERNAL indirect vector table
_KERNAL void _VECTOR_READ(uintptr_t vec[16])      { uint8_t x,y; __asm__("SEC\nJSR $FF8D":CLOB_X,CLOB_Y:"x"(_LO(vec)),"y"(_HI(vec)):"a","c","memory"); }
_KERNAL void _VECTOR_SET(const uintptr_t vec[16]) { uint8_t x,y; __asm__ volatile("CLC\nJSR $FF8D":CLOB_X,CLOB_Y:"x"(_LO(vec)),"y"(_HI(vec)):"a","c"); }

// Enable or mute printing of system error and control messages. If bit 7 is set in flags, error messages are enabled. If bit 6 is set in flags, control messages are enabled.
_KERNAL void _SETMSG(uint8_t flags) { __asm__ volatile("JSR $FF90"::"a"(flags):); }

// Send a Secondary Address to a Device on the Serial Bus after LISTEN
_KERNAL void _SECOND(uint8_t addr)  { uint8_t a; __asm__ volatile("JSR $FF93":CLOB_A:"a"(addr):); }

// Send a Secondary Address to a Device on the Serial Bus after TALK
_KERNAL void _TKSA(uint8_t addr)    { uint8_t a; __asm__ volatile("JSR $FF96":CLOB_A:"a"(addr):); }

// Read/Set Top of RAM Pointer
_KERNAL uint16_t _MEMTOP_READ() { uint8_t lo, hi; __asm__("SEC\nJSR $FF99":"=x"(lo),"=y"(hi)::"c"); return _U16(lo, hi); }
_KERNAL void _MEMTOP_SET(uint16_t membot) { uint8_t x,y; __asm__ volatile("CLC\nJSR $FF99":CLOB_X,CLOB_Y:"x"(_LO(membot)),"y"(_HI(membot)):"c"); }

// Read/Set Bottom of RAM Pointer
_KERNAL uint16_t _MEMBOT_READ() { uint8_t lo, hi; __asm__("SEC\nJSR $FF9C":"=x"(lo),"=y"(hi)::"c"); return _U16(lo, hi); }
_KERNAL void _MEMBOT_SET(uint16_t membot) { uint8_t x,y; __asm__ volatile("CLC\nJSR $FF9C":CLOB_X,CLOB_Y:"x"(_LO(membot)),"y"(_HI(membot)):"c"); }

// Scan keyboard
_KERNAL void _SCNKEY()                       { __asm__ volatile("JSR $FF9F":::"a","x","y"); }

// Set time-out flag on serial bus
_KERNAL void _SETTMO(uint8_t flag)           { __asm__ volatile("JSR $FFA2"::"a"(flag):); }

// Input byte from serial port
_KERNAL uint8_t _ACPTR()       { uint8_t byte; __asm__ volatile("JSR $FFA5":"=a"(byte)::"x"); return byte; }

// Output byte to serial port
_KERNAL void _CIOUT(uint8_t byte) { uint8_t a; __asm__ volatile("JSR $FFA8":CLOB_A:"a"(byte):); }

// Send UNTALK command to serial bus
_KERNAL void _UNTLK()                        { __asm__ volatile("JSR $FFAB":::"a"); }

// Send UNLISTEN command to serial bus
_KERNAL void _UNLSN()                        { __asm__ volatile("JSR $FFAE":::"a"); }

// Send LISTEN command to serial bus
_KERNAL void _LISTEN(uint8_t device) { uint8_t a; __asm__ volatile("JSR $FFB1":CLOB_A:"a"(device):); }

// Send TALK command to serial bus
_KERNAL void _TALK(uint8_t device)   { uint8_t a; __asm__ volatile("JSR $FFB4":CLOB_A:"a"(device):); }

// Return I/O status byte
_KERNAL uint8_t _READST() { uint8_t status; __asm__("JSR $FFB7":"=a"(status)::); return status; }

// Set logical file #, device #, secondary # for I/O
_KERNAL void _SETLFS(uint8_t file_no, uint8_t device_address, uint8_t secondary_address) { __asm__ volatile("JSR $FFBA"::"a"(file_no),"x"(device_address),"y"(secondary_address):); }

// Sets filename for OPEN command
_KERNAL void _SETNAM(const char *filename, uint8_t length) { __asm__ volatile("JSR $FFBD"::"a"(length), "x"(_LO(filename)), "y"(_HI(filename)):); }

// Open a Logical File
_KERNAL uint8_t _OPEN() { uint8_t error; __asm__ volatile("JSR $FFC0":"=a"(error)::"x","y","c"); return error; }

// Close a Logical File
_KERNAL uint8_t _CLOSE(uint8_t file_no)  { uint8_t error; __asm__ volatile("JSR $FFC3":"=a"(error):"a"(file_no):"x","y","c"); return error; }

// Designate a Logical File as the current Input Channel
_KERNAL uint8_t _CHKIN(uint8_t file_no)  { uint8_t error,x; __asm__ volatile("JSR $FFC6":"=a"(error),CLOB_X:"x"(file_no):"y","c"); return error; }

// Designate a Logical File As the current Output Channel
_KERNAL uint8_t _CHKOUT(uint8_t file_no) { uint8_t error,x; __asm__ volatile("JSR $FFC9":"=a"(error),CLOB_X:"x"(file_no):"y","c"); return error; }

// Restore current Input and Output Devices to the Default Devices
_KERNAL void _CLRCHN()                     {                __asm__ volatile("JSR $FFCC":::"a","x"); }

// Input a character from the current Input Channel
_KERNAL uint8_t _CHRIN()                   { uint8_t byte;  __asm__ volatile("JSR $FFCF":"=a"(byte)::"c"); return byte; }

// Output a character to the current Output Channel
_KERNAL void _CHROUT(uint8_t byte)         {                __asm__ volatile("JSR $FFD2"::"a"(byte):"c"); }

// Load file to RAM, or verify (compare) file contents against RAM. To use, call SETNAM+SETLFS+LOAD. This function
// does not open a file handle, so no call to CLOSE is needed afterwards.
// Note: Only files stored with the SAVE function may be loaded in this way. Files stored with the character-based
// OPEN+CHKOUT+CHROUT+CLOSE method can not be loaded with this function.
_KERNAL uint8_t _LOAD(uint8_t verify, void *dst, void **dst_end)
{
  uint8_t error, lo, hi;
  __asm__ volatile("JSR $FFD5":"=a"(error), "=x"(lo), "=y"(hi):"a"(verify),"x"(_LO(dst)),"y"(_HI(dst)):"c","memory");
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
  __asm__ volatile(
    "LDA #%[zp]\n"
    "JSR $FFD8\n" // Call the 'SAVE' KERNAL ROM routine
    "BCS error\n"
    "LDA #0\n"
    "error:\n"
    :"=a"(error),CLOB_X,CLOB_Y:[zp]"r"(start),"x"(_LO(end)),"y"(_HI(end)):"c");
  return error;
}

// Set the current real-time clock time ("Jiffies counter")
typedef _BitInt(24) _Jiffies;
_KERNAL void _SETTIM(_Jiffies jiffies) { __asm__ volatile("JSR $FFDB"::"a"((uint8_t)(jiffies>>16)),"x"((uint8_t)(jiffies>>8)),"y"((uint8_t)(jiffies))); }

// Read the RTC clock
_KERNAL _Jiffies _RDTIM()
{
  uint8_t lo, mid, hi;
  __asm__("JSR $FFDE":"=a"(hi),"=x"(mid),"=y"(lo)::);
  return ((_Jiffies)hi << 16) | ((_Jiffies)mid << 8) | lo;
}

// Query Stop key indicator. If pressed, call CLRCHN and clear keyboard buffer.
_KERNAL uint8_t _STOP()  { uint8_t stop; __asm__ volatile("JSR $FFE1\nTSX\nTXA\nAND #2":"=a"(stop)::"x"); return stop; }

// Wait to get a character from current Input Channel (compare to _CHRIN, which does not pause to wait, but returns EOF if buffer is empty)
// Returns PETSCII code of character.
_KERNAL uint8_t _GETIN() { uint8_t byte; __asm__ volatile("JSR $FFE4":"=a"(byte)::"x","y","c"); return byte; }

// Close All Logical I/O Files. (does not actually close, but discards track of all open files, as if they were closed)
_KERNAL void _CLALL()    {               __asm__ volatile("JSR $FFE7":::"a","x"); }

// Update the RTC Clock and Check for the STOP Key (called regularly by KERNAL)
_KERNAL void _UDTIM()    {               __asm__ volatile("JSR $FFEA":::"a","x"); }

// Get number of screen rows/columns
_KERNAL void _SCREEN(uint8_t *width, uint8_t *height) { __asm__("JSR $FFED":"=x"(*width),"=y"(*height)::"a","memory"); }

// Read/Set current text cursor position
_KERNAL void _PLOT_READ(uint8_t *x, uint8_t *y)       { __asm__("SEC\nJSR $FFF0":"=x"(*x),"=y"(*y)::"c","memory"); }
_KERNAL void _PLOT_SET(uint8_t x, uint8_t y)          { __asm__ volatile("CLC\nJSR $FFF0"::"x"(y),"y"(x):"c"); }

// Fetch CIA #1 base address
_KERNAL uintptr_t _IOBASE() { uint8_t lo, hi;           __asm__("JSR $FFF3":"=x"(lo),"=y"(hi)::); return _U16(lo, hi); }

#endif // ~__C64__
