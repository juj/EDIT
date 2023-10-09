#ifndef _LINE_H
#define _LINE_H

#include "memory.h"
#include "highlite.h"

// Line::colorsFlags bitfield
#define LINE_STARTS_IN_COMMENT 0x01
#define LINE_ENDS_IN_COMMENT 0x02

struct Line
{
  unsigned ownsPtr : 1;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  unsigned char colorFlags;
#endif
  long length;
  long capacity;
  char huge *text;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  unsigned char huge *colors;
#endif
  Line far *prevLine;
  Line far *nextLine;
};

Line far *advance_n_lines(Line far *startingLine, long numLinesForward);
Line far *get_nth_line_of_document(long lineNumber); // Linear search from start of doc to find Nth line
Line far *get_nth_line_on_screen(long screenY); // Linear search from start of screen to find Nth line

long screen_end_y_of_last_line_on_screen(Line far *startingLine, int startingScreenY);

// Returns 1-based line number
long get_line_number(Line far *line);
long get_current_line_number();

void delete_line(Line far *line);
int increase_capacity(Line far *line, long newCapacity);

int line_comes_before(Line far *a, Line far *b);

// Returns number of lines from a to b, or -1 if a does not precede b.
int calculate_line_delta(Line far *a, Line far *b);

char last_nonws_char_on_line(Line far *line, long fromPosition);

int find_line_indent_position(Line far *line);

// Compares whether line1:col1 comes before or after line2:col2.
// Returns -1 if before, +1 if after and 0 if they are equal.
int cursor_pos_cmp(unsigned long line1, unsigned long col1, unsigned long line2, unsigned long col2);

#endif
