#ifndef _FILE_H
#define _FILE_H

#include "memory.h"
#include <stdio.h>

struct Line;

struct File
{
  char *filename;
  char huge *fileData;
  long size;
  Line far *firstLine;
  Line far *lastLine;
  int hasUnsavedChanges;

  File *prevFile;
  File *nextFile;
};

File *open_new_file();

long huge_fread(char huge *dst, long numBytes, FILE *handle);
File *load_file(const char *filename);
void save_file(File *file);
void close_file(File *file);

#endif
