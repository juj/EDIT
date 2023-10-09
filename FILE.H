#ifndef _FILE_H
#define _FILE_H

#include "memory.h"
#include "types.h"
#include <stdio.h>

struct Line;

struct File
{
  char filename[MAX_FILENAME_LEN+1];
  char huge *fileData;
  file_idx size;
  Line far *firstLine;
  Line far *lastLine;
  int hasUnsavedChanges;

  File *prevFile;
  File *nextFile;
};

File *open_new_file();

#ifdef __mos__
struct FILE;
#endif

void mark_current_file_modified();

bool is_file_unnamed(File *f);

file_idx calculate_file_byte_size(File *f);

file_idx huge_fread(char huge *dst, file_idx numBytes, FILE *handle);
file_idx huge_fwrite(char huge *src, file_idx numBytes, FILE *handle);
File *load_file(const char *filename);
void save_file(File *file);
void close_file(File *file);

#endif
