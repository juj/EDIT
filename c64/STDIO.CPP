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

#if defined(_STDIO_DEBUG_LEVEL) && _STDIO_DEBUG_LEVEL >= 2
#define _STDIO_DEBUG2(...) Printf(__VA_ARGS__)
#else
#define _STDIO_DEBUG2(...) ((void)0)
#endif

bool string_contains_char(const char *str, char ch)
{
  assert(str);
  while(*str) if (*str++ == ch) return true;
  return false;
}

static uint8_t extract_device_number_from_string(const char *str)
{
  assert(str);
  while(*str) if (isdigit(*str)) return atoi(str);
  return 8; // Default to first disk drive
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

static uint8_t read_next_char(FILE *f, bool *error)
{
  if (f->nextCharAvailable)
  {
    f->nextCharAvailable = *error = false;
    return f->nextChar;
  }

  if (f->error) // Possibly at EOF?
  {
    *error = true;
    return f->error;
  }
  // For best performance, we would like to keep calling CHRIN() in as tight loop
  // as possible, and observe disk errors only via the carry flag returned by CHRIN.
  // However, if a disk is at EOF, issuing a CHRIN will not raise the
  // error flag, but instead on error, and a decimal value 199 is returned.
  // Presumably the intent is that caller should always check READST before issuing
  // a read call, but that might have slower performance. (TBD benchmark)
  // So an idea: instead of checking READST before reading a char with CHRIN(),
  // we proceed to CHRIN without READST. CHRIN is observed to return decimal 199
  // when at end of file, so if that happens, we then call READST to confirm if we
  // are actually at EOF.
  // (todo: while the above worked for disk drives, looks like it didn't go so well
  // for Datasette. Consolidate this code between Datasette and disk drive)
  /*
  uint8_t err = DRIVE_STATUS_ERROR();
  if (err)
  {
    f->error = err;
    *error = true;
    return 0;
  }
  */
  uint8_t ch = _CHRIN_GET_ERROR(error);
  if (*error) _STDIO_DEBUG2("FREAD ERR");
/////  if (ch == 199)
  {
    uint8_t err = DRIVE_STATUS_ERROR();
    if (err)
    {
      // On C64 Datasette, the EOF error flag behaves maybe
      // slightly counterintuitively:
      // when we see EOF after a CHRIN, it means that the character
      // returned by CHRIN is still actually valid, and EOF is right
      // after this character. In such case, we should not perform
      // any subsequent reads to the file.
      // As a subtle gotcha, calling CLRCHN after Datasette EOF
      // will make the C64 not remember that the Datasette was at
      // EOF, so attempting a subsequent CHKIN+CHRIN will cause
      // the C64 to display a "Press Play On Tape" message to
      // continue reading. Therefore we must locally record
      // the EOF flag in memory to mark this file handle at EOF,
      // but not react to the EOF right here. Then subsequent read
      // attempts can look at the error flag (including EOF flag)
      // and observe that the tape is now at EOF.
      f->error = err;
      if (err & ~0x40) // If we have any other error than EOF?
        *error = true;
    }
  }
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
  assert(filename);
  assert(mode);
  static uint8_t fileHandleCounter = 1;

  char fn[MAX_FILENAME_LEN+3+4]; // Add +3 for "@0:" prefix and +4 for ",U,W" suffix.
  // Prepend "@0:" to the filename to have any existing filename be replaced.
  fn[0] = 64; // '@' in PETSCII
  fn[1] = '0';
  fn[2] = ':';
  uint8_t device_number = extract_device_number_from_string(mode);
  bool reading_file = string_contains_char(mode, 'r');
  bool accessing_disk_drive = (device_number >= 8);
  uint8_t i = accessing_disk_drive ? 3 : 0;
  for(uint8_t j = 0; filename[j]; ++j) { fn[i++] = filename[j]; }
  if (accessing_disk_drive)
  {
    // Append ",S,R" or ",S,W" to filename to tell that the file type is Sequential, and that
    // we are either reading or writing to the file.
    fn[i++] = ',';
    fn[i++] = string_contains_char(mode, 'p') ? 'P' : 'S';
    fn[i++] = ',';
    fn[i++] = reading_file ? 'R' : 'W';
  }
  _SETNAM(fn, i);
  _SETLFS(fileHandleCounter, device_number, accessing_disk_drive ? 2 : (reading_file ? 0 : 1));
  uint8_t error = _OPEN();
  print_dos_error(error, "FOPEN");

  error = reading_file ? _CHKIN(fileHandleCounter) : _CHKOUT(fileHandleCounter);
  print_dos_error(error, "FOPEN/CHKINOUT");

  if ((error = _READST()))
  {
    _CLOSE(fileHandleCounter);
    _CLRCHN();
    _STDIO_DEBUG2("FOPEN/READST ERROR");
    return 0;
  }

  FILE *f = (FILE*)malloc(sizeof(FILE));
  if (!f)
  {
    _CLRCHN();
    _STDIO_DEBUG2("FOPEN OOM");
    return 0;
  }
  f->nextCharAvailable = false;
  f->handle = fileHandleCounter++; // TODO multiple handles
  f->error = 0;
  if (fileHandleCounter >= 127) fileHandleCounter = 1;

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

static void fscanf_skip_whitespace(FILE *f)
{
  bool error;
  uint8_t ch = read_next_char(f, &error);
  while(!error && isspace(ch))
    ch = read_next_char(f, &error);
  if (!error) put_char_back(f, ch); // Non-whitespace character, put it back to stream
}

static int fscanf_read_int(FILE *f)
{
  bool error;
  uint8_t ch = read_next_char(f, &error);
  char intstr[6] = { 0, 0, 0, 0, 0, 0 }; // max length 65536
  uint8_t i = 0;
  assert(isdigit(ch)); // Precondition to calling this function: next character in file must be an integer
  while(isdigit(ch))
  {
    intstr[i++] = ch;
    if (i >= 5) i = 0; // if i gets this large, read will overflow (will read garbage)
    ch = read_next_char(f, &error);
    if (error) goto end;
  }
  put_char_back(f, ch);
end:
  return atoi(intstr);
}

static void fscanf_read_str(FILE *f, char *dst)
{
  bool error;
  uint8_t ch = read_next_char(f, &error);
  assert(!isspace(ch));
  while(!isspace(ch))
  {
    *dst++ = ch;
    ch = read_next_char(f, &error);
    if (error) return;
  }
  put_char_back(f, ch);
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
  fscanf_skip_whitespace(f);

  for(; *format; ++format)
  {
    if (*format != '%')
    {
      ch = read_next_char(f, &error);
      if (error) goto end;
      if (ch != *format)
      {
        put_char_back(f, ch);
        goto end;
      }
      // A single space character in format should
      // consume as many whitespaces from input as possible
      if (isspace(*format)) fscanf_skip_whitespace(f);
    }
    else
    {
      ++format;
      switch(*format)
      {
      case 'd':
      case 'u': // TODO test if this actually works?
      {
        ch = read_next_char(f, &error);
        put_char_back(f, ch);
        if (error) goto end;
        if (!isdigit(ch)) goto end;
        int *dst = va_arg(args, int*);
        *dst = fscanf_read_int(f);
        ++num_parsed;
      }
      break;
      case 's':
      {
        ch = read_next_char(f, &error);
        if (error) goto end;
        if (isspace(ch)) goto end; // todo not 100% sure if this would happen. Maybe "100asdf" being scanned as "%d %s"?
        char *str = va_arg(args, char*);
        fscanf_read_str(f, str);
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

  if (f->error) return 0;
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
  const uint8_t *ptr = (const uint8_t*)buffer;
  for(size_t i = 0; i < size; ++i)
    for(size_t j = 0; j < count; ++j)
    {
      if (DRIVE_STATUS_ERROR()) break;
      _CHROUT(*ptr++);
    }
  _CLRCHN();
  return (uintptr_t)ptr - (uintptr_t)buffer;
}

int fputc(int ch, FILE *f)
{
  assert(f);
  if (_CHKOUT(f->handle) || DRIVE_STATUS_ERROR()) return EOF;
  bool error = _CHROUT_GET_ERROR((unsigned char)ch);
  _CLRCHN();
  return error ? EOF : ch;
}

int fputs(const char *str, FILE *f)
{
  assert(str);
  assert(f);

  if (_CHKOUT(f->handle)) return EOF;

  while(*str)
  {
    bool error = _CHROUT_GET_ERROR(*str++);
    if (error)
    {
      _CLRCHN();
      return EOF;
    }
  }
  _CLRCHN();
  return 0;
}

/*
// Prints the given uint8 to CHROUT.
void chrout_u8(uint8_t num)
{
  uint8_t zp0, zp1;
  // Convert 'num' to 2x2 BCD digits.
  __asm__(R"(
    SED // Enter Binary Coded Decimal (BCD) mode (affects ADC commands below)

    LDA #0
    STA %[zp0] // zp0-zp1 will store the output BCD digits, 0-255.
    STA %[zp1] // Initialize these all to zero to start

    LDX #8 // Loop each of the 8 bits of the u8 input
loop%=:
    ROL %[input]   // Shift the input u8 one bit to the left

    LDA %[zp0] // Multiply the three BCD number pairs by two, and add the input bit from carry to the BCD
    ADC %[zp0]
    STA %[zp0]
    LDA %[zp1]
    ADC %[zp1]
    STA %[zp1]

    DEX     // Loop back to next input bit
    BNE loop%=

    CLD // Exit BCD mode
  )":[zp0]"=r"(zp0), [zp1]"=r"(zp1), [input]"+r"(num)::"a","x","c");

  if (zp1)     goto three;
  if (zp0 > 9) goto two;
  goto one;
three: _CHROUT('0'+(zp1&0xF));
two:   _CHROUT('0'+(zp0>>4));
one:   _CHROUT('0'+(zp0&0xF));
}
*/

// Prints the given uint16 to CHROUT.
void chrout_u16(uint16_t num)
{
  uint8_t zp0, zp1, zp2;
  // Convert 'num' to 3x2 BCD digits.
  __asm__(R"(
    SED // Enter Binary Coded Decimal (BCD) mode (affects ADC commands below)

    LDA #0
    STA %[zp0] // zp0-zp2 will store the output BCD digits, 0-65535.
    STA %[zp1] // Initialize these all to zero to start
    STA %[zp2]

    LDX #16 // Loop each of the 16 bits of the u16 input
loop%=:
    ROL %[input]   // Shift the input u16 one bit to the left
    ROL %[input]+1 // rotating the next MSB bit of input into Carry

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
    BNE loop%=

    CLD // Exit BCD mode
  )":[zp0]"=r"(zp0), [zp1]"=r"(zp1), [zp2]"=r"(zp2), [input]"+r"(num)::"a","x","c");

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

void chrout_x16(uint16_t num)
{
  const uint8_t ch[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  if (num >= 0x1000) goto four;
  if (num >= 0x100) goto three;
  if (num >= 0x10) goto two;
  goto one;
four:  _CHROUT(ch[num>>12]);
three: _CHROUT(ch[(num>>8)&15]);
two:   _CHROUT(ch[(num>>4)&15]);
one:   _CHROUT(ch[num&15]);
}

void chrout_X16(uint16_t num)
{
  const uint8_t ch[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
  if (num >= 0x1000) goto four;
  if (num >= 0x100) goto three;
  if (num >= 0x10) goto two;
  goto one;
four:  _CHROUT(ch[num>>12]);
three: _CHROUT(ch[(num>>8)&15]);
two:   _CHROUT(ch[(num>>4)&15]);
one:   _CHROUT(ch[num&15]);
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
    if (*format != '%')
    {
      _CHROUT(*format == '\n' ? '\r' : *format);
      continue;
    }

    ++format;
    switch(*format)
    {
    // TODO: %x/%X/%04X/%lu/%ld
    case '%': _CHROUT('%'); break;
    case 'c': _CHROUT(va_arg(args, int/*char*/)); break;
    case 'd':
      dst = va_arg(args, int);
      if (dst < 0) { _CHROUT('-'); dst = -dst; }
      chrout_u16(dst);
      __asm__("NOP":::"memory"); // xxx random fix to mask/avoid bug https://github.com/llvm-mos/llvm-mos/issues/393
    break;
    case 'u':
      dst = va_arg(args, unsigned int);
      chrout_u16(dst);
      __asm__("NOP":::"memory"); // xxx random fix to mask/avoid bug https://github.com/llvm-mos/llvm-mos/issues/393
    break;
    case 'x':
      chrout_x16(va_arg(args, unsigned int));
      __asm__("NOP":::"memory"); // xxx random fix to mask/avoid bug https://github.com/llvm-mos/llvm-mos/issues/393
      break;
    case 'X':
      chrout_X16(va_arg(args, unsigned int));
      __asm__("NOP":::"memory"); // xxx random fix to mask/avoid bug https://github.com/llvm-mos/llvm-mos/issues/393
      break;
    case 's':
      str = va_arg(args, const char *);
      assert(str);
      while(*str) _CHROUT(*str++);
    break;
    default: assert(false); break;
    }
  }
  va_end(args);
}

char *strstr(const char *str1, const char *str2)
{
  while(*str1)
  {
    while(*str1 != *str2)
    {
      ++str1;
      if (!*str1) return 0;
    }
    const char *s1 = str1;
    const char *s2 = str2;
    while(*s1 == *s2)
    {
      ++s1;
      ++s2;
      if (!*s1) return !*s2 ? (char*)str1 : 0;
      if (!*s2) return (char*)str1;
    }
    ++str1;
  }
  return 0;
}

static unsigned char petscii_to_screen_code(unsigned char petscii)
{
  if (petscii <= 31) return 32;//petscii + 128;
  if (petscii <= 63) return petscii;
  if (petscii <= 95) return petscii - 64;
  if (petscii <= 127) return petscii - 32;
  if (petscii <= 159) return 32;//petscii + 64;
  if (petscii <= 191) return petscii - 64;
  if (petscii <= 254) return petscii - 128;
  return 0x5E;
}

char *gets_s(char *str, rsize_t n)
{
  assert(str);
  assert(n > 0);
  str[n-1] = 0; // Null-terminate input early
  --n;
  uint8_t x, y; _PLOT_READ(&x, &y);
  uint8_t w, h; _SCREEN(&w, &h);
  // TODO: Read the base address (it can be configured to change)
  const uint16_t cursor_ofs = (uint16_t)y*w+x;
  volatile uint8_t * const cursor_start_addr = (uint8_t*)0x0400 + cursor_ofs;
  volatile uint8_t * const color_ram = (uint8_t*)0xD800 + cursor_ofs;
  volatile uint8_t * const cursor_blink = (uint8_t *)0xCC;
  uint8_t old_cursor_blink = *cursor_blink;
  *cursor_blink = 0; // blink cursor

  uint8_t color = *(volatile uint8_t *)0x286; // Use the cursor color
  color_ram[0] = color; // Default LTBLUE

  int8_t i = 0;
  for(;;)
  {
    _PLOT_SET(x+i, y);
    uint8_t keyCode = 0;
    while(!keyCode) keyCode = _GETIN();
    switch(keyCode)
    {
    case 0x5F: *cursor_blink = old_cursor_blink; return 0; // ESC
    case 0x14: case 0x94: // DEL_INST
      if (i > 0)
      {
        cursor_start_addr[--i] = ' ';
        _PLOT_SET(x+i, y);
        cursor_start_addr[i+1] &= 0x7F;
      }
      break;
    case 0x0D: case 0x8D:
      str[i] = 0;
      _PLOT_SET(0, y+1); // Move cursor to next line
      cursor_start_addr[i] &= 0x7F;
      *cursor_blink = old_cursor_blink; 
      return str; // 0D: RETURN, 8D: RETURN while holding down shift (or shift-lock)
    default:
      if (i < n)
      {
        cursor_start_addr[i] = petscii_to_screen_code(keyCode);
        str[i++] = keyCode;
        color_ram[i] = color;
      }
      break;
    }
  }
}
