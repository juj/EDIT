#include "../file.h"
#include "../line.h"
#include "../input.h"
#include <string.h>
#include <stdint.h>

static void SETNAM(const char *filename, uint8_t filenameLength)
{
  asm volatile("JSR $FFBD\n" // FFBD = SETNAM
                : // outputs
                : "a"(filenameLength), "x"((unsigned char)(uintptr_t)filename), "y"((unsigned char)((uintptr_t)filename>>8))
                : "a", "x", "y");
}

static void SETLFS(unsigned char logicalFileNum, unsigned char deviceNum, unsigned char secondaryAddr)
{
  asm volatile("JSR $FFBA\n" // FFBA = SETLFS
                : // outputs
                : "a"(logicalFileNum), "x"(deviceNum), "y"(secondaryAddr)
                : "a", "x", "y");
}

// https://www.c64-wiki.com/wiki/Zeropage: $0002 is unused so safe to access.
//static volatile unsigned char * const tmp = (unsigned char *)0x0002; // arbitrary safe temporary memory address for getting data out of inline assembly

static uint8_t OPEN()
{
  unsigned char errorCode;
  asm volatile("JSR $FFC0\n" // FFC0 = OPEN
//               "BCS .return\n"
  //             "LDA #0\n"
    //           ".return\n"
//               "STA $0002\n"
                : "=a" (errorCode) // outputs
                : // inputs
                : "x", "y", "c");  
  return errorCode;
}

static void CLOSE(unsigned char logicalFileNum)
{
  asm volatile("JSR $FFC3\n" // FFC3 = CLOSE
                : // outputs
                : "a"(logicalFileNum) // inputs
                : "a", "x", "y");
}

static void CHKOUT(unsigned char logicalFileNum)
{
  asm volatile("JSR $FFC9\n" // FFC9 = CHKOUT
                : // outputs
                : "x"(logicalFileNum)
                : "x");
}

static uint8_t READST()
{
  unsigned char statusByte;
  asm volatile("JSR $FFB7\n" // FFB7 = READST Read I/O status byte
//               "STA $0002\n"
                : "=a" (statusByte) // outputs
                : // inputs
                : "a");
  return statusByte;
}

static uint8_t CHROUT(uint8_t byteToWrite)
{
  unsigned char error;
  asm volatile("JSR $FFD2\n" // FFD2 = CHROUT
               "LDA #0\n"
               "ROL\n"
//               "STA $0002\n"
                : "=a"(error) // outputs
                : "a"(byteToWrite)
                : "a", "c");  
  return error;
}

static void CLRCHN()
{
  asm volatile("JSR $FFCC\n" // FFCC = CLRCHN - Restore default I/O channels
                : // outputs
                : // inputs
                : "a", "x");
}

void save_file(File *file)
{
  char fn[MAX_FILENAME_LEN+1+3+4]; // Add +1 for null, +3 for "@0:" prefix and +4 for ",U,W" suffix.
  // Prepend "@0:" to the filename to have any existing filename be replaced.
  fn[0] = 64; // '@' in PETSCII
  fn[1] = '0';
  fn[2] = ':';
  uint8_t i = 3;
  while(file->filename[i]) { fn[i] = file->filename[i]; ++i; }
  // Append ",S,W" to filename to tell that the file type is Sequential, and that
  // we are Writing to the file.
  fn[i++] = ',';
  fn[i++] = 'S';
  fn[i++] = ',';
  fn[i++] = 'W';
  fn[i] = 0;
  SETNAM(fn, i);
  SETLFS(2, 8, 2);
  uint8_t error = OPEN();
  // 1541 documentation: https://archive.org/details/Commodore_1541_Disk_Drive_Users_Guide_1982-09_Commodore/page/n57/mode/2up
  // "Error message numbers less than 20 should be ignored with the exception of 01 which gives information about the number of
  // "files scratched" (we are not calling the SCRATCH command here)
  // Oddly I am getting an error 199, so ignore too high error numbers as well. (something is very definitely wrong)
  if (error >= 20 && error <= 74)
  {
    printf("ERROR: %d\n", (int)error);
// TODO: how to do this:
//      ; check drive error channel here to test for
//        ; FILE EXISTS error etc.
    get_key();
    return;
  }
  CHKOUT(2);  

  Line *l = file->firstLine;
  while(l)
  {
    for(uint16_t i = 0; i < (uint16_t)l->length; ++i)
    {
      if ((error = READST())) goto end;
      if ((error = CHROUT(l->text[i]))) goto end;
    }
    if ((error = READST())) goto end;
    if ((error = CHROUT('\n'))) goto end;
    l = l->nextLine;
  }
end:
  CLOSE(2);
  CLRCHN();

  if (error)
  {
    printf("FAILED %d\n", error);
    get_key();
  }
  else
    file->hasUnsavedChanges = 0;
}
