#include "mystdio.h"
#include "c64_kernal.h"

#ifndef EOF
#define EOF -1 // xxx todo move to header after upstream stdio.h is removed
#endif

// If enabled, file read and write operations briefly flash the screen border
#define STDIO_FLASH_BORDERS_VISUALIZATION

#ifdef STDIO_FLASH_BORDERS_VISUALIZATION
static volatile uint8_t *const screen_border = (uint8_t*)0xD020;

class SaveOldScreenBorder
{
#ifdef STDIO_FLASH_BORDERS_VISUALIZATION
public:
  uint8_t border;
  SaveOldScreenBorder():border(*screen_border) {}
  ~SaveOldScreenBorder() { *screen_border = border; }
#endif
};

#define SCOPE_SAVE_SCREEN_BORDER() SaveOldScreenBorder _s
#define SET_SCREEN_BORDER(ch) (*screen_border = (ch))
#else
#define SCOPE_SAVE_SCREEN_BORDER() ((void)0)
#define SET_SCREEN_BORDER(ch) ((void)0)
#endif

void __assert(bool condition, const char *str, const char *file, int line, const char *func)
{
#ifndef NDEBUG
  if (!condition)
  {
    // TODO: Don't call printf from within assert, but instead do a super simple putscreen
    // to print this info.
#if defined(_STDIO_DEBUG_LEVEL) && _STDIO_DEBUG_LEVEL >= 2
    printf("\nFILE %s:%d:%s: ASSERT \"%s\" FAILED!\n", file, line, func, str);
#elif defined(_STDIO_DEBUG_LEVEL) && _STDIO_DEBUG_LEVEL >= 1
    printf("\nFILE %s:%d: ASSERT FAILED!\n", file, line);
#else
    printf("\nLINE %d: ASSERT FAILED!\n", line);
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
#define _STDIO_DEBUG2(...) printf(__VA_ARGS__)
#else
#define _STDIO_DEBUG2(...) ((void)0)
#endif

bool string_contains_char(const char *str, char ch)
{
  assert(!(ch & 0x20));
  assert(str);
  while(*str) if ((*str++ & ~0x20) == ch) return true;
  return false;
}

static uint8_t extract_device_number_from_string(const char *str)
{
  assert(str);
  while(*str) { if (isdigit(*str)) return atoi(str); ++str; }
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
      printf("%s: ERROR %u\n", operation, (unsigned int)error);
      return;
  }
  printf("%s: %s\n", operation, err);
#elif _STDIO_DEBUG_LEVEL >= 1
  printf("%s: ERROR %u\n", operation, (unsigned int)error);
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
  SET_SCREEN_BORDER(ch);
  // TODO/Bug workaround? In my editor test code, if I first save a cpp file
  // to tape, then attempting to load it does not register an EOF condition
  // correctly, but the tape reads a 0 after the EOF, and then reads an
  // indefinite number of 0x20 bytes. EOF does not seem to be received
  // in all cases properly. So to work around this, treat read 0 byte
  // to mean EOF. This means that we can only write SEQ text files to cassette
  // via this function, and those can't have the null byte in them.
  // TODO: IS this condition a bug in my code, or some limitation/issue in
  // the C64 KERNAL?
  if (ch == 0 && f->deviceNum == 1)
  {
    // Treat received null byte from the Datassette an EOF condition.
    *error = f->error = 0x40; // EOF
    return 0;
  }

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

extern "C" FILE *fopen(const char *filename, const char *mode)
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
  bool writing_file = string_contains_char(mode, 'w' & ~0x20);
  bool accessing_disk_drive = (device_number >= 8);
  uint8_t i = accessing_disk_drive ? 3 : 0;
  for(uint8_t j = 0; filename[j]; ++j) { fn[i++] = filename[j]; }
  if (accessing_disk_drive)
  {
    // Append ",S,R" or ",S,W" to filename to tell that the file type is Sequential, and that
    // we are either reading or writing to the file.
    fn[i++] = ',';
    fn[i++] = string_contains_char(mode, 'p' & ~0x20) ? 'P' : 'S';
    fn[i++] = ',';
    fn[i++] = writing_file ? 'W' : 'R';
  }
  _SETNAM(fn, i);
  _SETLFS(fileHandleCounter, device_number, accessing_disk_drive ? 2 : writing_file);
  uint8_t error = _OPEN();
  print_dos_error(error, "FOPEN");

  error = writing_file ? _CHKOUT(fileHandleCounter) : _CHKIN(fileHandleCounter);
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
  f->deviceNum = device_number;
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
    if (error)
    {
      *dst = 0;
      return;
    }
  }
  *dst = 0;
  put_char_back(f, ch);
}

extern "C" int fscanf(FILE *f, const char *format, ...)
{
  if (_CHKIN(f->handle)) return 0;
  SCOPE_SAVE_SCREEN_BORDER();

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
        char *str = va_arg(args, char*);
        fscanf_read_str(f, str);
        ++num_parsed;
      }
      break;
      case 'c':
      {
        char *dst = va_arg(args, char*);
        *dst = read_next_char(f, &error);
        if (error) goto end;
        ++num_parsed;
      }
      break;
      default: assert(false);
      }
    }
  }
end:
  va_end(args);
  _CLRCHN();
  return num_parsed;
}

extern "C" char *fgets(char *str, int count, FILE *f)
{
  assert(str);
  assert(count > 0);
  assert(f);

  if (f->error) return 0;
  if (_CHKIN(f->handle)) return 0;
  SCOPE_SAVE_SCREEN_BORDER();

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

extern "C" int fgetc(FILE *f)
{
  assert(f);
  if (_CHKIN(f->handle)) return EOF;
  SCOPE_SAVE_SCREEN_BORDER();

  bool error;
  uint8_t ch = read_next_char(f, &error);
  _CLRCHN();
  if (error) return EOF;
  else return ch;
}

extern "C" size_t fread(void *buffer, size_t size, size_t count, FILE *f)
{
  if (_CHKIN(f->handle)) return 0;
  SCOPE_SAVE_SCREEN_BORDER();

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

extern "C" size_t fwrite(const void *buffer, size_t size, size_t count, FILE *f)
{
  assert(f);
  if (_CHKOUT(f->handle)) return 0;
  SCOPE_SAVE_SCREEN_BORDER();

  const uint8_t *ptr = (const uint8_t*)buffer;
  for(size_t i = 0; i < size; ++i)
    for(size_t j = 0; j < count; ++j)
    {
      if (DRIVE_STATUS_ERROR()) break;
      SET_SCREEN_BORDER(*ptr);
      _CHROUT(*ptr++);
    }
  _CLRCHN();
  return (uintptr_t)ptr - (uintptr_t)buffer;
}

extern "C" size_t fwrite8(const void *buffer, uint8_t count, FILE *f)
{
  assert(f);
  if (_CHKOUT(f->handle)) return 0;
  SCOPE_SAVE_SCREEN_BORDER();

  const uint8_t *ptr = (const uint8_t*)buffer;
  while(count--)
  {
    if (DRIVE_STATUS_ERROR()) break;
    SET_SCREEN_BORDER(*ptr);
    _CHROUT(*ptr++);
  }
  _CLRCHN();
  return (uintptr_t)ptr - (uintptr_t)buffer;
}

extern "C" int fputc(int ch, FILE *f)
{
  assert(f);
  if (_CHKOUT(f->handle) || DRIVE_STATUS_ERROR()) return EOF;
  SCOPE_SAVE_SCREEN_BORDER();
  SET_SCREEN_BORDER(ch);

  bool error = _CHROUT_GET_ERROR((unsigned char)ch);
  _CLRCHN();
  return error ? EOF : ch;
}

extern "C" int fputs(const char *str, FILE *f)
{
  assert(str);
  assert(f);

  if (_CHKOUT(f->handle)) return EOF;
  SCOPE_SAVE_SCREEN_BORDER();

  while(*str)
  {
    SET_SCREEN_BORDER(*str);
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

extern "C" int puts(const char *str)
{
  _CLRCHN();
  while(*str) _CHROUT(*str++);
  _CHROUT('\r');
  return 1;
}

extern "C" void *memcpy(void *dest, const void *src, size_t count) // TODO: optimize
{
  assert((uintptr_t)dest >= 0x2);
  assert((uintptr_t)src >= 0x2);
  assert(count < 40000); // Arbitrary limit to indicate a programming error
  char *d = (char*)dest;
  const char *s = (const char *)src;
  while(count--) *d++ = *s++;
  return dest;
}

extern "C" void *memcpy8(void *dest, const void *src, uint8_t count) // TODO: optimize
{
  assert((uintptr_t)dest >= 0x2);
  assert((uintptr_t)src >= 0x2);
  char *d = (char*)dest;
  const char *s = (const char *)src;
  while(count--) *d++ = *s++;
  return dest;
}

extern "C" void *memmove(void *dest, const void *src, size_t num)
{
  if (dest < src || !num) return memcpy(dest, src, num);

  char *d = (char*)dest + (num-1);
  char *s = (char*)src + (num-1);
  do *d-- = *s--; while(d != dest);
  return dest;
}

extern "C" void putchar(char c) // TODO: optimize
{
  if (__builtin_expect(c == '\n', 0))
    c = '\r';
  _CHROUT(c);
}

extern "C" char *strcat(char *s1, const char *s2) // TODO: optimize
{
  size_t len = strlen(s1);
  strcpy(s1 + len, s2);
  return s1;
}

extern "C" int strcmp(const char *s1, const char *s2)
{
  for(;;)
  {
    if (!*s1) return -1;
    if (!*s2) return 1;
    if (!*s1 != !*s2) return *s1 - *s2;
    ++s1;
    ++s2;
  }
}

extern "C" int strncmp(const char *s1, const char *s2, size_t num) // TODO: optimize
{
  for(const char *end = s1 + num; s1 < end;)
  {
    if (!*s1) return -1;
    if (!*s2) return 1;
    if (*s1 != *s2) return *s1 - *s2;
    ++s1;
    ++s2;
  }
  return 0;
}

extern "C" void *memset(void *ptr, int value, size_t num) // TODO: optimize
{
  char *p = (char*)ptr;
  for(; num; --num) *p++ = (char)value;
  return ptr;
}

extern "C" char *strcpy(char *s1, const char *s2) {
  char *ret = s1;
  for (;; ++s1, ++s2) {
    *s1 = *s2;
    if (!*s1)
      return ret;
  }
}

extern "C" size_t strlen(const char *s)
{
  const char *s2 = s;
  while(*s2) ++s2;
  return s2 - s;
}

extern "C" uint8_t strlen8(const char *s)
{
  uint8_t len = 0;
  while(s[len])
  {
    assert(len != 255);
    ++len;
  }
  return len;
}

template <typename T> static inline T _atoi(const char *str) {
  T result = 0;
  bool sign = false;

  while (isspace(*str)) { str++; }

  if (*str == '+') { str++; }
  else if (*str == '-') { sign = true; str++; }

  while (*str >= '0' && *str <= '9') {
    result = (result * 10) + (*(str++) - '0');
  }
  return sign ? -result : result;
}

extern "C" {
int atoi(const char *str) { return _atoi<int>(str); }
long atol(const char *str) { return _atoi<long>(str); }
}

extern "C" int isalpha(int c) { // TODO: optimize
  char ch = (char)c;
  return ((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z'));
}

extern "C" int isdigit(int c) { // TODO: optimize
  char ch = (char)c;
  return ch >= '0' && ch <= '9';
}

extern "C" int isspace(int c) { // TODO: optimize
  char ch = c;
  return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\f') ||
         (ch == '\r') || (ch == '\v');
}

extern "C" int isalnum(int c) { return isalpha(c) || isdigit(c); } // TODO: optimize

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

// Prints the given uint32 to CHROUT. (max 42_94_96_72_95)
void chrout_u32(uint32_t num)
{
  volatile uint32_t *tmp_u32 = (uint32_t*)0xCBF7;
  *tmp_u32 = num;
  uint8_t zp0, zp1, zp2, zp3, zp4;
  // Convert 'num' to 5x2 BCD digits.
  __asm__(R"(
    SED // Enter Binary Coded Decimal (BCD) mode (affects ADC commands below)

    LDA #0
    STA %[zp0] // zp0-zp4 will store the output BCD digits, 0-4294967295.
    STA %[zp1] // Initialize these all to zero to start
    STA %[zp2]
    STA %[zp3]
    STA %[zp4]

    LDX #32 // Loop each of the 32 bits of the u32 input
loop%=:
    ROL $CBF7 // Shift the input u32 one bit to the left
    ROL $CBF8 // rotating the next MSB bit of input into Carry
    ROL $CBF9 // rotating the next MSB bit of input into Carry
    ROL $CBFA // rotating the next MSB bit of input into Carry

    LDA %[zp0] // Multiply the three BCD number pairs by two, and add the input bit from carry to the BCD
    ADC %[zp0]
    STA %[zp0]
    LDA %[zp1]
    ADC %[zp1]
    STA %[zp1]
    LDA %[zp2]
    ADC %[zp2]
    STA %[zp2]
    LDA %[zp3]
    ADC %[zp3]
    STA %[zp3]
    LDA %[zp4]
    ADC %[zp4]
    STA %[zp4]

    DEX     // Loop back to next input bit
    BNE loop%=

    CLD // Exit BCD mode
  )":[zp0]"=r"(zp0), [zp1]"=r"(zp1), [zp2]"=r"(zp2), [zp3]"=r"(zp3), [zp4]"=r"(zp4)::"a","x","c");

  if (zp4 > 9) goto ten;
  if (zp4)     goto nine;
  if (zp3 > 9) goto eight;
  if (zp3)     goto seven;
  if (zp2 > 9) goto six;
  if (zp2)     goto five;
  if (zp1 > 9) goto four;
  if (zp1)     goto three;
  if (zp0 > 9) goto two;
  goto one;
ten:   _CHROUT('0'+(zp4>>4));
nine:  _CHROUT('0'+(zp4&0xF));
eight: _CHROUT('0'+(zp3>>4));
seven: _CHROUT('0'+(zp3&0xF));
six:   _CHROUT('0'+(zp2>>4));
five:  _CHROUT('0'+(zp2&0xF));
four:  _CHROUT('0'+(zp1>>4));
three: _CHROUT('0'+(zp1&0xF));
two:   _CHROUT('0'+(zp0>>4));
one:   _CHROUT('0'+(zp0&0xF));
}

char *itoa_u16(char *dst, uint16_t num)
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
five:  *dst++ = '0'+zp2;
four:  *dst++ = '0'+(zp1>>4);
three: *dst++ = '0'+(zp1&0xF);
two:   *dst++ = '0'+(zp0>>4);
one:   *dst++ = '0'+(zp0&0xF);
  return dst;
}

// Prints the given uint32 to CHROUT. (max 42_94_96_72_95)
char *itoa_u32(char *dst, uint32_t num)
{
  volatile uint32_t *tmp_u32 = (uint32_t*)0xCBF7;
  *tmp_u32 = num;
  uint8_t zp0, zp1, zp2, zp3, zp4;
  // Convert 'num' to 5x2 BCD digits.
  __asm__(R"(
    SED // Enter Binary Coded Decimal (BCD) mode (affects ADC commands below)

    LDA #0
    STA %[zp0] // zp0-zp4 will store the output BCD digits, 0-4294967295.
    STA %[zp1] // Initialize these all to zero to start
    STA %[zp2]
    STA %[zp3]
    STA %[zp4]

    LDX #32 // Loop each of the 32 bits of the u32 input
loop%=:
    ROL $CBF7 // Shift the input u32 one bit to the left
    ROL $CBF8 // rotating the next MSB bit of input into Carry
    ROL $CBF9 // rotating the next MSB bit of input into Carry
    ROL $CBFA // rotating the next MSB bit of input into Carry

    LDA %[zp0] // Multiply the three BCD number pairs by two, and add the input bit from carry to the BCD
    ADC %[zp0]
    STA %[zp0]
    LDA %[zp1]
    ADC %[zp1]
    STA %[zp1]
    LDA %[zp2]
    ADC %[zp2]
    STA %[zp2]
    LDA %[zp3]
    ADC %[zp3]
    STA %[zp3]
    LDA %[zp4]
    ADC %[zp4]
    STA %[zp4]

    DEX     // Loop back to next input bit
    BNE loop%=

    CLD // Exit BCD mode
  )":[zp0]"=r"(zp0), [zp1]"=r"(zp1), [zp2]"=r"(zp2), [zp3]"=r"(zp3), [zp4]"=r"(zp4)::"a","x","c");

  if (zp4 > 9) goto ten;
  if (zp4)     goto nine;
  if (zp3 > 9) goto eight;
  if (zp3)     goto seven;
  if (zp2 > 9) goto six;
  if (zp2)     goto five;
  if (zp1 > 9) goto four;
  if (zp1)     goto three;
  if (zp0 > 9) goto two;
  goto one;
ten:   *dst++ = '0'+(zp4>>4);
nine:  *dst++ = '0'+(zp4&0xF);
eight: *dst++ = '0'+(zp3>>4);
seven: *dst++ = '0'+(zp3&0xF);
six:   *dst++ = '0'+(zp2>>4);
five:  *dst++ = '0'+(zp2&0xF);
four:  *dst++ = '0'+(zp1>>4);
three: *dst++ = '0'+(zp1&0xF);
two:   *dst++ = '0'+(zp0>>4);
one:   *dst++ = '0'+(zp0&0xF);
  return dst;
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

char *itox_x16(char *dst, uint16_t num)
{
  const uint8_t ch[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  if (num >= 0x1000) goto four;
  if (num >= 0x100) goto three;
  if (num >= 0x10) goto two;
  goto one;
four:  *dst++ = ch[num>>12];
three: *dst++ = ch[(num>>8)&15];
two:   *dst++ = ch[(num>>4)&15];
one:   *dst++ = ch[num&15];
  return dst;
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

char *itoX_X16(char *dst, uint16_t num)
{
  const uint8_t ch[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
  if (num >= 0x1000) goto four;
  if (num >= 0x100) goto three;
  if (num >= 0x10) goto two;
  goto one;
four:  *dst++ = ch[num>>12];
three: *dst++ = ch[(num>>8)&15];
two:   *dst++ = ch[(num>>4)&15];
one:   *dst++ = ch[num&15];
  return dst;
}

extern "C" int printf(const char *format, ...)
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
    break;
    case 'u':
      dst = va_arg(args, unsigned int);
      chrout_u16(dst);
    break;
    case 'x':
      chrout_x16(va_arg(args, unsigned int));
      break;
    case 'X':
      chrout_X16(va_arg(args, unsigned int));
      break;
    case 's':
      str = va_arg(args, const char *);
      assert(str);
      while(*str) _CHROUT(*str++);
      break;
    case 'l':
      switch(*++format)
      {
      case 'd':
      case 'D':
      {
        int32_t i32 = va_arg(args, int32_t);
        if (i32 < 0) { _CHROUT('-'); i32 = -i32; }
        chrout_u32(i32);
      }
      break;
      case 'u':
      case 'U':
      {
        uint32_t u32 = va_arg(args, uint32_t);
        chrout_u32(u32);
      }
      break;
      default: assert(false); break;
      }
    break;
    default: assert(false); break;
    }
  }
  va_end(args);
  return 1;
}

extern "C" int sprintf(char *s, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int dst;
  const char *str;

  for(; *format; ++format)
  {
    if (*format != '%')
    {
      *s++ = (*format == '\n' ? '\r' : *format);
      continue;
    }

    ++format;
    switch(*format)
    {
    // TODO: %x/%X/%04X/%lu/%ld
    case '%': *s++ = '%'; break;
    case 'c': *s++ = va_arg(args, int/*char*/); break;
    case 'd':
      dst = va_arg(args, int);
      if (dst < 0) { *s++ = '-'; dst = -dst; }
      s = itoa_u16(s, dst);
      break;
    case 'u':
      dst = va_arg(args, unsigned int);
      s = itoa_u16(s, dst);
      break;
    case 'x':
      s = itox_x16(s, va_arg(args, unsigned int));
      break;
    case 'X':
      s = itoX_X16(s, va_arg(args, unsigned int));
      break;
    case 's':
      str = va_arg(args, const char *);
      assert(str);
      while(*str) *s++ = *str++;
      break;
    case 'l':
      switch(*++format)
      {
      case 'd':
      case 'D':
      {
        int32_t i32 = va_arg(args, int32_t);
        if (i32 < 0) { *s++ = '-'; i32 = -i32; }
        s = itoa_u32(s, i32);
      }
      break;
      case 'u':
      case 'U':
      {
        uint32_t u32 = va_arg(args, uint32_t);
        s = itoa_u32(s, u32);
      }
      break;
      default: assert(false); break;
      }
      break;
    default: assert(false); break;
    }
  }
  va_end(args);
  *s++ = 0;
  return 1;
}

extern "C" char *strstr(const char *str1, const char *str2)
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

extern "C" char *gets_s(char *str, rsize_t n)
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

extern "C" int memcmp(const void *ptr1, const void *ptr2, size_t num)
{
  const unsigned char *p1 = (const unsigned char *)ptr1;
  const unsigned char *p2 = (const unsigned char *)ptr2;
  while(num)
  {
    int ch = (int)*p1 - (int)*p2;
    if (ch) return ch;
    ++p1; ++p2; --num;
  }
  return 0;
}

#include "reucpy.h"

static volatile uint8_t *const reu_status = (uint8_t*)0xDF00;
static volatile uint8_t *const reu_command = (uint8_t*)0xDF01;
static volatile uintptr_t *const reu_c64_addr = (uintptr_t*)0xDF02;
static volatile reu_addr_t *const reu_cart_addr = (reu_addr_t*)0xDF04;
static volatile uint16_t *const reu_xfer_length = (uint16_t*)0xDF07;
static volatile uint8_t *const reu_address_ctl = (uint8_t*)0xDF0A;

void reucpy(void *c64_addr, unsigned _BitInt(24) reu_addr, uint16_t size, uint8_t direction)
{
  assert(direction == RAM2REU || direction == REU2RAM);
  // Call reu_detect() first if this assert() triggers, or if manually
  // having done REU accesses in between, be sure to reset bits 7 and 6
  // of REU address control register back to zero.
  assert((*reu_address_ctl & 0xC0) == 0);

  *reu_c64_addr = (uintptr_t)c64_addr;
  *reu_cart_addr = reu_addr;
  *reu_xfer_length = size;
  *reu_command = direction;
}

int reu_detect()
{
  *reu_address_ctl = *reu_address_ctl & ~0xC0; // Make both C64 and REU addresses advance per each transferred byte

  char data[5] = { 'a', 'r', 'e', 'u', 0 };
  for(int bank = 255; bank >= 0; --bank)
  {
    data[0] = (uint8_t)bank;
    reucpy(data, (reu_addr_t)bank << 16, 4, RAM2REU);
  }
  memset(data, 0, sizeof(data));
  for(int bank = 0; bank < 256; ++bank)
  {
    reucpy(data, (reu_addr_t)bank << 16, 4, REU2RAM);
    if (data[0] != bank || data[1] != 'r' || data[2] != 'e' || data[3] != 'u') return bank;
  }
  return 256; // Full 16MB of REU available
}
