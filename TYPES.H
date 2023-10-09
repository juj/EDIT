#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// Maximum length of a filename (excluding null terminator)
#ifdef __C64__
#define MAX_FILENAME_LEN 16
#elif defined(__MSDOS__)
#define MAX_FILENAME_LEN 12 // 8.3
#else
#define MAX_FILENAME_LEN 256
#endif

// Maximum # of characters in a single file
#define MAX_CHARS_IN_FILE 65534
typedef uint16_t file_idx;
#define FILE_IDX_PRINTF_MOD "%u"

// Maximum # of characters on a single line
#define MAX_CHARS_PER_LINE 254
typedef uint8_t line_xpos;

// Maximum number of lines in a document
#define MAX_LINES_IN_FILE 32760
typedef int16_t line_num;
#define LINE_NUM_PRINTF_MOD "%d"

// Type to store cursor x or y position on screen
typedef int8_t cursor_pos;

#endif
