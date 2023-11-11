#pragma once

// Before including this file, call
// #define _STDIO_DEBUG_LEVEL 2 to add verbose debugging support, or
// #define _STDIO_DEBUG_LEVEL 1 to add more lightweight debugging support.
// #define NDEBUG to disable assert() macro.
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

//#include_next <stdio.h>

//#define _STDIO_DEBUG_LEVEL 1

#ifndef _STDIO_DEBUG_LEVEL
#define _STDIO_DEBUG_LEVEL 0
#endif

typedef _BitInt(24) int24_t;

struct FILE
{
  uint8_t nextChar; // for read streams, the next character to come out from the file.
  bool nextCharAvailable; // If true, nextChar is present. If false, must read first.
  uint8_t handle;
  uint8_t error;
};

#define MAX_FILENAME_LEN 16

bool string_contains_char(const char *str, char ch);

void print_dos_error(uint8_t error, const char *operation);

uint8_t read_next_char(FILE *f);

void consume_read_whitespace(FILE *f);

int read_int_from_file(FILE *f);

void read_string_from_file(FILE *f, char *dst);

#ifdef __cplusplus
extern "C" {
#endif

char *strstr(const char *str1, const char *str2);

// Call fopen("", "1r"); to load any next file from cassette, or fopen("FOO.TXT", "1r"); to load the next filename that matches this file.
// Pass character 'p' in fopen() when writing a PRG file, otherwise writing a SEQ file.
FILE *fopen(const char *filename, const char *mode);
int fscanf(FILE *f, const char *format, ...);
int fgetc(FILE *f);
char *fgets(char *str, int count, FILE *f);
size_t fread(void *buffer, size_t size, size_t count, FILE *f);
size_t fwrite(const void *buffer, size_t size, size_t count, FILE *f);
int fputc(int ch, FILE *f);
int fputs(const char *str, FILE *f);
int fclose(FILE *f);

int printf(const char *format, ...);

void __assert(bool condition, const char *str, const char *file, int line, const char *func);

#ifdef NDEBUG
#define assert(x) ((void)0)
#elif _STDIO_DEBUG_LEVEL >= 2
#define assert(x) __assert((x), #x, __FILE__, __LINE__, __func__);
#elif _STDIO_DEBUG_LEVEL >= 1
#define assert(x) __assert((x), "", __FILE__, __LINE__, "");
#else
#define assert(x) __assert((x), "", "", __LINE__, "");
#endif

typedef uint8_t rsize_t;
char *gets_s(char *str, rsize_t n);

#ifdef __cplusplus
} // extern "C"
#endif
