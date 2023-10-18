#include "mystdio.h"
#include "c64_kernal.h"

#ifndef EOF
#define EOF -1 // xxx todo move to header after upstream stdio.h is removed
#endif

//#define _STDIO_DEBUG_LEVEL 2

void __assert(bool condition, const char *str, const char *file, int line, const char *func)
{
#ifndef NDEBUG
  if (!condition)
  {
    // TODO: Don't call printf from within assert, but instead do a super simple putscreen
    // to print this info.
#if defined(_STDIO_DEBUG_LEVEL) && _STDIO_DEBUG_LEVEL >= 2
    Printf("%s:%d:%s: ASSERT \"%s\" FAILED!\n", file, line, func, str);
#elif defined(_STDIO_DEBUG_LEVEL) && _STDIO_DEBUG_LEVEL >= 1
    Printf("%s:%d: ASSERT FAILED!\n", file, line);
#else
    Printf("LINE %d: ASSERT FAILED!\n", line);
#endif
    for(;;) // Halt, and blink border red-black
    {
      *(volatile uint8_t*)0xD020 = (*(volatile uint8_t*)0xD020) & 0xF ? 0 : 0x0A;
      for(unsigned int i = 0; i < 65535; ++i) asm("NOP":::"memory"); // pause for a moment
    }
  }
#endif
}

bool string_contains_char(const char *str, char ch)
{
  assert(str);
  while(*str) if (*str++ == ch) return true;
  return false;
}

void print_dos_error(uint8_t error, const char *operation)
{
#if _STDIO_DEBUG_LEVEL >= 1
  if (error < 20) return;
  _CLRCHN();
#endif

#if _STDIO_DEBUG_LEVEL >= 2
  const char *err;
  switch(error)
  {
    // From https://archive.org/details/Commodore_1541_Disk_Drive_Users_Guide_1982-09_Commodore/page/n59/mode/2up
    case 20: err = "BLOCK HEADER NOT FOUND"; break; // The disk controUer is unable to locate the header of the requested data block. Caused by an illegal block number, or the header has been destroyed.
    case 21: err = "SYNC CHARACTER NOT FOUND"; break; // The disk controller is unable to detect a sync mark on the desired track. Caused by misalignment of the read/ writer head, no diskette is present, or unformatted or improperly sealed diskette. Can also indicate a hardware failure.
    case 22: err = "DATA BLOCK NOT PRESENT"; break; // The disk controller has been requested to read or verify a data block that was not properly written. This error message occurs in conjunction with the BLOCK commands and indicates an illegal track and/or block request.
    case 23: err = "CHECKSUM ERROR"; break; // This error message indicates that there is an error in one or more of the data bytes. The data has been read into the DOS memory, but the checksum over the data is in error. This message may also indicate grounding problems.
    case 24: err = "BYTE DECODING ERROR"; break; // The data or header as been read into the DOS memory, but a hardware error has been created due to an invalid bit pattern in the data byte. This message may also indicate grounding problems.
    case 25: err = "WRITE-VERIFY ERROR"; break; // This message is generated if the controller detects a mismatch between the written data and the data in the DOS memory.
    case 26: err = "WRITE PROTECT ON"; break; // This message is generated when the controller has been requested to write a data block while the write protect switch is depressed. Typically, this is caused by using a diskette with a write a protect tab over the notch.
    case 27: err = "HEADER CHECKSUM ERROR"; break; // The controller has detected an error in the header of the requested data block. The block has not been read into the DOS memory. This message may also indicate grounding problems.
    case 28: err = "DATA EXTENDS INTO NEXT BLOCK"; break; // The controller attempts to detect the sync mark of the next header after writing a data block. If the sync mark does not appear within a pre-determined time, the error message is generated. The error is caused by a bad diskette format (the data extends into the next block), or by hardware failure.
    case 29: err = "DISK ID MISMATCH"; break; // This message is generated when the controller has been requested to access a diskette which has not been initialized. The message can also occur if a diskette has a bad header.
    case 30: err = "GENERAL SYNTAX ERROR"; break; // The DOS cannot interpret the command sent to the command channel. Typically, this is caused by an illegal number of file names, or patterns are illegally used. For example, two file names may appear on the left side of the COPY command.
    case 31: err = "INVALID COMMAND"; break; // The DOS does not recognize the command. The command must start in the first position,
    case 32: err = "LONG LINE"; break; // The command sent is longer than 58 characters.
    case 33: err = "INVALID FILENAME"; break; // Pattern matching is invalidly used in the OPEN or SAVE command.
    case 34: err = "NO FILE GIVEN"; break; // The file name was left out of a command or the DOS does not recognize it as such. Typically, a colon (:) has been left out of the command.
    case 39: err = "COMMAND FILE NOT FOUND"; break; // This error may result if the command sent to command channel (secondary address 15) is unrecognized by the DOS.
    case 50: err = "RECORD NOT PRESENT"; break; // Result of disk reading past the last record through 1NPUT#, or GET# commands. This message will also occur after positioning to a record beyond end of file in a relative file. If the intent is to expand the file by adding the new record (with a PRINTS command), the error message may be ignored. INPUT or GET should not be attempted after this error is detected without first repositioning.
    case 51: err = "OVERFLOW"; break; // PRINTS statement exceeds record boundary. Information is cut off. Since the carriage return is sent as a record terminator is counted in the record size. This message will occur if the total characters in the record (including the final carriage return) exceeds the defined size.
    case 52: err = "FILE TOO LARGE"; break; // Record position within a relative file indicates that disk overflow will result.
    case 60: err = "FILE OPEN FOR WRITE"; break; // This message is generated when a write file that has not been closed is being opened for reading.
    case 61: err = "FILE NOT OPEN"; break; // This message is generated when a file is being accessed that has not been opened in the DOS. Sometimes, in this case, a message is not generated; the request is simply ignored.
    case 62: err = "FILE NOT FOUND"; break; // The requested file does not exist on the indicated drive.
    case 63: err = "FILE EXISTS"; break; // The file name of the file being created already exists on the diskette.
    case 64: err = "FILE TYPE MISMATCH"; break; // The file type does not match the file type in the directory entry for the requested file.
    case 65: err = "NO BLOCK"; break; // This message occurs in conjunction with the B-A command. It indicates that the block to be allocated has been previously allocated. The parameters indicate the track and sector available with the next highest number. If the parameters are zero (0), then all blocks higher in number arc in use.
    case 66: err = "ILLEGAL TRACK OR SECTOR"; break; // The DOS has attempted to access a irack or block which does not e>dsi in die format being used. This may indicate a problem reading the pointer to the next block.
    case 67: err = "ILLEGAL SYSTEM TRACK OR SECTOR"; break; // This special error message indicates an illegal system track or block.
    case 70: err = "NO CHANNELS AVAILABLE"; break; // The requested channel is not available, or all channels are in use. A maximum of five sequential files may be opened at one time to the DOS, Direct access channels may have six opened files.
    case 71: err = "DIRECTORY ERROR"; break; // The BAM docs not match the internal count. There is a problem in the BAM allocation or the BAM has been overwritten in DOS memory. To correct this problem, reinitialize the diskette to restore the BAM in memory. Some active files may be terminated by the corrective action. NOTE: BAM = Block Availability Map
    case 72: err = "DISK OR DIRECTORY FULL"; break; // Either the blocks on the diskette are used or the directory is at its entry limit. DISK FULL is sent when two blocks are available on the 1541 to allow the current file to be closed.
    case 73: err = "POWER UP MESSAGE; break; OR WRITE ATTEMPT WITH DOS MISMATCH"; break; // DOS I and 2 are read compatible but not write compatible. Disks may be interchangeably read with either DOS, but a disk formatted on one version cannot be written upon with the other version because the format is different. This error is displayed whenever an attempt is made to write upon a disk which has been formatted in a non-compatible format. (A utility routine is available to assist in converting from one format to another.) This message may also appear after power up.
    case 74: err = "DRIVE NOT READY"; break; // An attempt has been made to access the 1541 Single Drive Floppy Disk without any diskettes present in either drive.
    default:
      Printf("%s: ERROR %u\n", operation, (unsigned int)error);
      return;
  }
  Printf("%s: %s\n", operation, err);
#elif _STDIO_DEBUG_LEVEL >= 1
  Printf("%s: ERROR %u\n", operation, (unsigned int)error);
#endif
}

// For some reason, bit 5 (checksum error) is seen to be
// high in the drive status flags field. Ignore it when
// testing for drive status errors
#define DRIVE_STATUS_ERROR() (_READST() & ~0x20)

uint8_t read_next_char(FILE *f, bool *error)
{
  if (f->nextCharAvailable)
  {
    f->nextCharAvailable = *error = false;
    return f->nextChar;
  }
  uint8_t ch = _CHRIN_GET_ERROR(error);
  if (ch == 199 && DRIVE_STATUS_ERROR()) // Value 199 is returned when end of file.
    *error = true; // We are at EOF
  return ch;
}

void put_char_back(FILE *f, uint8_t ch)
{
  assert(!f->nextCharAvailable);
  f->nextCharAvailable = true;
  f->nextChar = ch;
}

FILE *fopen(const char *filename, const char *mode)
{
  static uint8_t fileHandleCounter = 3;

  char fn[MAX_FILENAME_LEN+3+4]; // Add +3 for "@0:" prefix and +4 for ",U,W" suffix.
  // Prepend "@0:" to the filename to have any existing filename be replaced.
  fn[0] = 64; // '@' in PETSCII
  fn[1] = '0';
  fn[2] = ':';
  uint8_t i = 3;
  assert(filename);
  for(uint8_t j = 0; filename[j]; ++j) { fn[i++] = filename[j]; }
  // Append ",S,R" or ",S,W" to filename to tell that the file type is Sequential, and that
  // we are either reading or writing to the file.
  fn[i++] = ',';
  fn[i++] = 'S';
  fn[i++] = ',';
  bool readingFile = string_contains_char(mode, 'r');
  fn[i++] = readingFile ? 'R' : 'W';
  _SETNAM(fn, i);
  _SETLFS(fileHandleCounter, 8, fileHandleCounter); // todo file ID
  uint8_t error = _OPEN();
  print_dos_error(error, "FOPEN");

  error = readingFile ? _CHKIN(fileHandleCounter) : _CHKOUT(fileHandleCounter);
  print_dos_error(error, "FOPEN/CHKINOUT");

  if ((error = _READST()))
  {
    _CLOSE(fileHandleCounter);
    _CLRCHN();
    print_dos_error(error, "FOPEN/READST");
    return 0;
  }

  FILE *f = (FILE*)malloc(sizeof(FILE));
  if (!f)
  {
    _CLRCHN();
#if _STDIO_DEBUG_LEVEL >= 2
    Printf("FOPEN: OOM\n");
#endif
    return 0;
  }
  f->nextCharAvailable = false;
  f->handle = fileHandleCounter++; // TODO multiple handles
  if (fileHandleCounter == 255) fileHandleCounter = 2;

  _CLRCHN();
  return f;
}

int fclose(FILE *f)
{
  assert(f); // The behavior is undefined if the value of the pointer stream is used after fclose returns.
  _CLOSE(f->handle);
  free(f);
  return 0;
}

void consume_read_whitespace(FILE *f)
{
  bool error;
  uint8_t ch = read_next_char(f, &error);
  while(!error && isspace(ch))
    ch = read_next_char(f, &error);
  if (!error) put_char_back(f, ch); // Non-whitespace character, put it back to stream
}

int read_int_from_file(FILE *f)
{
  bool error;
  uint8_t ch = read_next_char(f, &error);
  char intstr[6] = { 0, 0, 0, 0, 0, 0 }; // max length 65536
  uint8_t i = 0;
  assert(isdigit(ch));
  while(isdigit(ch))
  {
    intstr[i++] = ch;
    if (i >= 5) i = 0; // if i gets this large, read will overflow (will read garbage)
    f->nextChar = read_next_char(f, &error);
    if (error)
    {
      //print_dos_error(f->error, "READINT");
      break;
    }
  }
  if (!error) put_char_back(f, ch);
  return atoi(intstr);
}

void read_string_from_file(FILE *f, char *dst)
{
  bool error;
  uint8_t ch = read_next_char(f, &error);
  assert(!isspace(ch));
  while(!isspace(ch))
  {
    *dst++ = ch;
    ch = read_next_char(f, &error);
    if (error)
    {
//      print_dos_error(f->error, "READSTR");
      break;
    }
  }
  if (!error) put_char_back(f, ch);
}

int fscanf(FILE *f, const char *format, ...)
{
  if (_CHKIN(f->handle)) return 0;

  va_list args;
  va_start(args, format);
  // consume any initial whitespace in input
  int8_t num_parsed = 0;
  uint8_t ch;
  bool error;
  consume_read_whitespace(f);

  for(; *format; ++format)
  {
    if (isspace(*format))
    {
      consume_read_whitespace(f);
      while(isspace(*format)) ++format;
    }
    else if (*format != '%')
    {
      ch = read_next_char(f, &error);
      if (error) goto end;
      if (ch != *format)
      {
        put_char_back(f, ch);
        goto end;
      }
    }
    else
    {
      ++format;
      switch(*format)
      {
      case 'd':
      {
        ch = read_next_char(f, &error);
        if (error) goto end;
        put_char_back(f, ch);
        if (!isdigit(ch)) goto end;
        int *dst = va_arg(args, int*);
        *dst = read_int_from_file(f);
        ++num_parsed;
      }
      break;
      case 's':
      {
        ch = read_next_char(f, &error);
        if (error) goto end;
        if (isspace(ch)) goto end; // todo not 100% sure if this would happen. Maybe "100asdf" being scanned as "%d %s"?
        char *str = va_arg(args, char*);
        read_string_from_file(f, str);
        ++num_parsed;
      }
      break;
      }
    }
  }
end:
  va_end(args);
  _CLRCHN();
  return num_parsed;
}

char *fgets(char *str, int count, FILE *f)
{
  assert(str);
  assert(count > 0);
  assert(f);

  if (_CHKIN(f->handle)) return 0;

  --count; // account early for null terminating byte
  char *dst = str;
  while(count-- > 0)
  {
    bool error;
    uint8_t ch = read_next_char(f, &error);
    if (error) break;
    *str++ = ch;
    if (ch == '\n') break;
  }
  *str = 0;
  _CLRCHN();
  return (dst == str) ? 0 : dst;
}

int fgetc(FILE *f)
{
  assert(f);
  if (_CHKIN(f->handle)) return EOF;
  bool error;
  uint8_t ch = read_next_char(f, &error);
  _CLRCHN();
  if (error) return EOF;
  else return ch;
}

size_t fread(void *buffer, size_t size, size_t count, FILE *f)
{
  if (_CHKIN(f->handle)) return 0;

  uint8_t *ptr = (uint8_t*)buffer;
  for(size_t i = 0; i < size; ++i)
    for(size_t j = 0; j < count; ++j)
    {
      bool error;
      uint8_t ch = read_next_char(f, &error);
      if (error) break;
      *ptr++ = ch;
    }
  _CLRCHN();
  return (uintptr_t)ptr - (uintptr_t)buffer;
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *f)
{
  assert(f);
  if (_CHKOUT(f->handle)) return 0;
//  print_dos_error(f->error, "FWRITE/CHKOUT");
  const uint8_t *ptr = (const uint8_t*)buffer;
  for(size_t i = 0; i < size; ++i)
    for(size_t j = 0; j < count; ++j)
    {
      if (DRIVE_STATUS_ERROR()) break;//{ print_dos_error(f->error, "FWRITE"); break; }
      _CHROUT(*ptr++);
    }
  _CLRCHN();
  return (uintptr_t)ptr - (uintptr_t)buffer;
}

int fputc(int ch, FILE *f)
{
  assert(f);

  if (_CHKOUT(f->handle)) return EOF;
//    print_dos_error(f->error, "FPUTC/CHKOUT");
  
  if (DRIVE_STATUS_ERROR())
  {
//    print_dos_error(f->error, "FPUTC");
    return EOF;
  }
  _CHROUT((unsigned char)ch);
  _CLRCHN();
  return ch;
}

int fputs(const char *str, FILE *f)
{
  assert(str);
  assert(f);

//  print_dos_error(f->error, "FPUTS/CHKOUT");
  if (_CHKOUT(f->handle)) return EOF;

  while(*str)
  {
    bool error = _CHROUT_GET_ERROR(*str++);
    if (error)
    {
//      print_dos_error(f->error, "FPUTS");
      _CLRCHN();
      return EOF;
    }
  }
  _CLRCHN();
  return 0;
}

// Prints the given uint16 to CHROUT.
void chrout_u16(uint16_t num)
{
  uint8_t zp0, zp1, zp2;
  __attribute__((leaf)) __asm__(R"(
    SED // Enter BCD mode (affects ADC commands below)

    LDA #0
    STA %[zp0] // Use three zero page registers to store the BCD output
    STA %[zp1] // Initialize these all to zero at start
    STA %[zp2]

    LDX #16 // Loop 16 bits of the u16 input
loop:
    ROL %[input] // Rotate u16 to left to get the next MSB bit of input into carry
    ROL %[input]+1

    LDA %[zp0] // Multiply the three BCD number pairs by two, and add the input bit from carry to the BCD
    ADC %[zp0]
    STA %[zp0]
    LDA %[zp1]
    ADC %[zp1]
    STA %[zp1]
    LDA %[zp2]
    ADC %[zp2]
    STA %[zp2]

    DEX     // Loop back to next input bit
    BNE loop

    CLD // Exit BCD mode
  )":[zp0]"=r"(zp0), [zp1]"=r"(zp1), [zp2]"=r"(zp2), [input]"+r"(num)::"a","x","c");

//  Printf("%02x %02x %02x\n", (int)zp0, (int)zp1, (int)zp2);
  if (zp2)     goto five;
  if (zp1 > 9) goto four;
  if (zp1)     goto three;
  if (zp0 > 9) goto two;
  goto one;
five:  _CHROUT('0'+ zp2);
four:  _CHROUT('0'+(zp1>>4));
three: _CHROUT('0'+(zp1&0xF));
two:   _CHROUT('0'+(zp0>>4));
one:   _CHROUT('0'+(zp0&0xF));
}

void Printf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int dst;
  const char *str;

  _CLRCHN();
  for(; *format; ++format)
  {
    if (*format == '%')
    {
      ++format;
      switch(*format)
      {
      // TODO: %x/%X/%04X/%lu/%ld
//      case '%': _CHROUT('%'); break;
  //    case 'c': _CHROUT(va_arg(args, int/*char*/)); break;
      case 'd':
        dst = va_arg(args, int);
        if (dst < 0) { _CHROUT('-'); dst = -dst; }
        goto print_unsigned;
      case 'u':
        dst = va_arg(args, unsigned int);
print_unsigned:
        chrout_u16(dst);
      break;
      case 's':
        str = va_arg(args, const char *);
        assert(str);
        while(!str) _CHROUT(*str++);
      break;
      default: assert(false); break;
      }
    }
    else
      _CHROUT(*format == '\n' ? '\r' : *format);
  }
  va_end(args);
}
