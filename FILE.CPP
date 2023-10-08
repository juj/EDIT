#include "file.h"
#include "line.h"
#include "util.h"
#include "screen.h"
#include "state.h"
#include "memory.h"
#include "viewport.h"
#include "audio.h"
#include <stdlib.h>
#include <string.h>
#include "iassert.h"

long huge_fread(char huge *dst, long numBytes, FILE *handle)
{
  long numSuccessfullyRead = 0;
#if defined(_MSC_VER) || defined(__TURBOC__)
  while(numBytes > 0)
  {
    long blockSize = MIN(numBytes, 1024);
    size_t numRead = fread(dst, 1, blockSize, handle);
    dst = &dst[blockSize];
    numSuccessfullyRead += (long)numRead;
    if (numRead != blockSize)
      break;
    numBytes -= blockSize;
  }
#endif
  return numSuccessfullyRead;
}

long huge_fwrite(char huge *src, long numBytes, FILE *handle)
{
  long numWritten = 0;
#if defined(_MSC_VER) || defined(__TURBOC__)
  while(numBytes > 0)
  {
    long blockSize = MIN(numBytes, 1024);
    size_t nW = fwrite(src, 1, blockSize, handle);
    src = &src[blockSize];
    numWritten += (long)nW;
    if (numWritten != blockSize)
      break;
    numBytes -= blockSize;
  }
#endif
  return numWritten;
}

int find_lines(File *f)
{
  char huge *data = f->fileData;
  assert(data);

  Line far *curLine = (Line far *)Farmalloc(sizeof(Line));
  if (!curLine)
  {
    // TODO: Deallocate file!?
    return 0;
  }
  int numLines = 1;
  curLine->ownsPtr = 0;
  curLine->prevLine = curLine->nextLine = 0;
  curLine->text = data;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  curLine->colors = 0;
  curLine->colorFlags = 0;
#endif
  f->firstLine = curLine;

  char huge *ch = data;
  long prevLineStart = 0;
  for(long i = 0; i < f->size; ++i)
  {
    if (showWhitespace)
    {
      if (ch[i] == '\t') ch[i] = CHAR_RIGHT_ARROW; // substitute tab chars with right arrows
      if (ch[i] == ' ') ch[i] = CHAR_MIDDLE_DOT;
    }

    if (ch[i] == 0x0A)
    {
      ++numLines;
      Line far *nextLine = (Line far *)Farmalloc(sizeof(Line));
      if (!nextLine)
      {
	      // TODO: Deallocate file!?
	      return 0;
      }
      nextLine->ownsPtr = 0;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
      nextLine->colors = 0;
      nextLine->colorFlags = 0;
#endif
      curLine->nextLine = nextLine;
      curLine->length = i - prevLineStart;
      curLine->capacity = curLine->length + 1; // '\n' adds to capacity.
      if (curLine->text[curLine->length-1] == 0x0D) // remove '\r'
        --curLine->length;
      prevLineStart = i+1;
      nextLine->prevLine = curLine;
      nextLine->nextLine = 0;
      nextLine->text = &ch[i+1];
      curLine = nextLine;
    }
  }
  // End the last line
  curLine->length = curLine->capacity = f->size - prevLineStart;
  f->lastLine = curLine;
  return 1;
}

File *open_new_file()
{
  File *f = (File*)Malloc(sizeof(File));
  if (!f)
    return 0;

  f->nextFile = f->prevFile = 0;
  f->hasUnsavedChanges = 0;
#if defined(__C64__) || defined(__MSDOS__) // files are all uppercase, and on C64 chars too
  const char *filename = "<UNTITLED>";
#else
  const char *filename = "<untitled>";
#endif
  filename = "TEST.CPP";
  f->filename = (char*)Malloc(strlen(filename)+1);
  if (!f->filename)
  {
    Free(f, sizeof(File));
    return 0;
  }

  strcpy(f->filename, filename);
  f->fileData = 0;
  f->size = 0;

  Line far *firstLine = (Line far *)Farmalloc(sizeof(Line));
  if (!firstLine)
  {
    Free(f, sizeof(File));
    Free(f->filename, strlen(filename)+1);
    return 0;
  }

  firstLine->ownsPtr = 1;
  firstLine->prevLine = firstLine->nextLine = 0;
  firstLine->text = 0;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  firstLine->colors = 0;
  firstLine->colorFlags = 0;
#endif
  f->firstLine = f->lastLine = firstLine;
  firstLine->length = firstLine->capacity = 0;

  return f;
}

void close_file(File *file)
{
  assert(file);
  if (file->hasUnsavedChanges)
  {
    NOTIMPL("Closing unsaved files (asking to save dialog) not yet implemented.");
    return;
  }
  if (file == currentFile)
  {
    File *newCurFile = file->nextFile ? file->nextFile : file->prevFile;
    if (!newCurFile)
      newCurFile = open_new_file();
    open_file_in_viewport(newCurFile, 1);
  }
  if (file->prevFile) file->prevFile->nextFile = file->nextFile;
  if (file->nextFile) file->nextFile->prevFile = file->prevFile;
  for(Line far *l = file->firstLine; l; l = l->nextLine)
  {
    if (l->ownsPtr)
      Farfree(l->text, l->capacity);
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
    if (l->colors) Farfree(l->colors, l->capacity);
#endif
  }

  Farfree(file->fileData, file->size);
  Free(file->filename, strlen(file->filename)+1);
  Free(file, sizeof(File));
}

File *load_file(const char *filename)
{
#if defined(_MSC_VER) || defined(__TURBOC__)
  FILE *handle = fopen(filename, "rb");
  if (!handle)
  {
    printf("Failed to open %s!\n", filename);
    return 0;
  }
  fseek(handle, 0, SEEK_END);
  long size = ftell(handle);
  fseek(handle, 0, SEEK_SET);

  File *f = (File*)Malloc(sizeof(File));
  if (!f)
  {
    fclose(handle);
    return 0;
  }
  f->nextFile = f->prevFile = 0;
  f->hasUnsavedChanges = 0;
  f->filename = (char*)Malloc(strlen(filename)+1);
  if (!f->filename)
  {
    fclose(handle);
    return 0;
  }
  strcpy(f->filename, filename);
  f->fileData = (char huge *)Farmalloc(size);
  if (!f->fileData)
  {
    fclose(handle);
    return 0;
  }
  f->size = size;
  size_t numRead = huge_fread(f->fileData, size, handle);
  if (numRead != size)
  {
    printf("Failed to read %ld bytes, only read %ld bytes!\n", size, (long)numRead);
    // todo: error handling
    return 0;
  }
  fclose(handle);

  int success = find_lines(f);
  if (!success)
  {
   // TODO:
//    deallocate_file(f);
    return 0;
  }
  return f;
#else
  return 0;
#endif
}

void save_file(File *file)
{
#if defined(_MSC_VER) || defined(__TURBOC__)
  assert(file);
  if (!strcmp(file->filename, "<unsaved>"))
  {
    NOTIMPL("Save As.. dialog not yet implemented");
    return;
  }
  Line far *line = file->firstLine;

  FILE *handle = fopen(file->filename, "wb");
  if (!handle)
    goto error;

  while(line)
  {
    long numWritten = huge_fwrite(line->text, line->length, handle);
    if (numWritten != line->length)
    {
      fclose(handle);
      goto error;
    }
    if (line->nextLine)
    {
      size_t written = fwrite("\r\n", 1, 2, handle);
      if (written != 2)
        goto error;
      line = line->nextLine;
    }
    else break;

  }
  fclose(handle);
  file->hasUnsavedChanges = 0;
  return;

error:
  char str[64];
  sprintf(str, "Unable to save file \"%s\"!", file->filename);
  log_error(str);
  return;
#endif
}

long calculate_file_byte_size(File *f)
{
  long size = 0;
  Line *l = f->firstLine;
  while(l)
  {
    size += l->length+1;
    l = l->nextLine;
  }
  return size;
}
