#include "file.h"
#include "line.h"
#include "util.h"
#include "screen.h"
#include "state.h"
#include "memory.h"
#include "viewport.h"
#include "audio.h"
#include "iassert.h"
#include "input.h"
#include <stdlib.h>
#include <string.h>

#ifdef __C64__
#include "c64/mystdio.h" // todo replace this
#endif

#if defined(_MSC_VER) || defined(__TURBOC__)

static long huge_fread(char huge *dst, long numBytes, FILE *handle)
{
  long numSuccessfullyRead = 0;
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
  return numSuccessfullyRead;
}

long huge_fwrite(char huge *src, long numBytes, FILE *handle)
{
  long numWritten = 0;
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
  return numWritten;
}
#endif

#if 0
line_num find_lines(File *f)
{
  char huge *data = f->fileData;
  assert(data);

  Line far *curLine = (Line far *)Farmalloc(sizeof(Line));
  if (!curLine)
  {
    // TODO: Deallocate file!?
    return 0;
  }
  line_num numLines = 1;
  curLine->ownsPtr = 0;
  curLine->prevLine = curLine->nextLine = 0;
  curLine->text = data;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  curLine->colors = 0;
  curLine->colorFlags = 0;
#endif
  f->firstLine = curLine;

  char huge *ch = data;
  file_idx prevLineStart = 0;
  for(file_idx i = 0; i < f->size; ++i)
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
#endif

void repaint_open_files_list(File *highlightFilename);

void mark_current_file_modified()
{
  if (!currentFile->hasUnsavedChanges)
  {
    currentFile->hasUnsavedChanges = 1;
    repaint_open_files_list(0);
  }
}

File *open_new_file()
{
  File *f = (File*)Malloc(sizeof(File));
  if (!f)
    return 0;
  memset(f, 0, sizeof(File));

#if defined(__C64__) || defined(__MSDOS__) // files are all uppercase, and on C64 chars too
  const char *filename = "<UNTITLED>";
#else
  const char *filename = "<untitled>";
#endif
  strcpy(f->filename, filename);

  Line far *firstLine = (Line far *)Farmalloc(sizeof(Line));
  if (!firstLine)
  {
    Free(f, sizeof(File));
    return 0;
  }

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
  if (file->hasUnsavedChanges && !ask_yes_no_prompt("CLOSE UNSAVED FILE (Y/N)? "))
    return;

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
    Farfree(l->text, l->capacity);
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
    if (l->colors) Farfree(l->colors, l->capacity);
#endif
  }

  Free(file, sizeof(File));
}

char ss[32];

File *load_file(const char *filename)
{
  FILE *handle = fopen(filename, "r");
  if (!handle)
  {
    printf("Failed to open %s!\n", filename);
    return 0;
  }

  File *f = (File*)Malloc(sizeof(File));
  if (!f)
  {
    fclose(handle);
    return 0;
  }
  memset(f, 0, sizeof(File));
  strcpy(f->filename, filename);

  char *tmp_line = (char*)malloc(MAX_CHARS_PER_LINE+1);
  if (!tmp_line)
  {
    free(f);
    fclose(handle);
    return 0;
  }
  bool prev_line_ended_in_newline = false;
  int y = 0; //zz
  while(fgets(tmp_line, MAX_CHARS_PER_LINE, handle))
  {
    line_xpos len = (line_xpos)strlen(tmp_line);
//    if (len == 1 && tmp_line[0] == 'G')
  //    break;
//////    sprintf(ss, "%d:%d", (int)len, (int)tmp_line[0]);
/////////    put_string_on_screen(ss, 0, y++, 0); // xxxx
    prev_line_ended_in_newline = (len > 0 && tmp_line[len-1] == '\n');
    if (prev_line_ended_in_newline) // subtract newline from line
    {
      tmp_line[len-1] = 0; // xxxx
      --len;
    }
//////    put_string_on_screen(tmp_line, 0, y++, 9); // xxxx

    Line far *curLine = (Line far *)Farmalloc(sizeof(Line));
    if (!curLine) break; // TODO Alert/report malloc error
    memset(curLine, 0, sizeof(Line));
    curLine->text = (char*)malloc(len);
    if (!curLine->text) break; // TODO Alert/report malloc error
    curLine->length = curLine->capacity = len;
    memcpy(curLine->text, tmp_line, len);
    if (!f->firstLine) f->firstLine = curLine;
    else
    {
      curLine->prevLine = f->lastLine;
      f->lastLine->nextLine = curLine;
    }
    f->lastLine = curLine;

    /* TODO:
    if (showWhitespace)
    {
      if (ch[i] == '\t') ch[i] = CHAR_RIGHT_ARROW; // substitute tab chars with right arrows
      if (ch[i] == ' ') ch[i] = CHAR_MIDDLE_DOT;
    }
    */
  }
  free(tmp_line);
  // Gotcha: Must append a dummy newline if the previous line ended in a newline
  // and the last line in file is actually empty.
/*
  if (prev_line_ended_in_newline)
  {
    Line far *lastLine = (Line far *)Farmalloc(sizeof(Line));
    if (lastLine)
    {
      memset(lastLine, 0, sizeof(Line));
      lastLine->length = lastLine->capacity = 0;
      lastLine->text = (char*)malloc(0);
      lastLine->prevLine = f->lastLine;
      f->lastLine->nextLine = lastLine;
      f->lastLine = lastLine;
    }
  }
*/
  fclose(handle);

#ifdef __C64__
  // XXXXXXXXXXXXXXXXXX HACK
  Line *last = f->lastLine;
  if (last->prevLine) last->prevLine->nextLine = 0;
  free(last);
  // XXXXXXXXXXXXXXXXXX HACK
#endif

  for(Line *l = f->firstLine; l; l = l->nextLine)
    syntax_highlight(l);
  get_key();
  return f;
}

bool is_file_unnamed(File *f)
{
  return f->filename[0] == 0 || f->filename[0] == '<';
}

file_idx calculate_file_byte_size(File *f)
{
  file_idx size = 0;
  Line *l = f->firstLine;
  while(l)
  {
    size += l->length+1;
    l = l->nextLine;
  }
  return size;
}
