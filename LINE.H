#ifndef _LINE_H
#define _LINE_H

#include "types.h"
#include "memory.h"
#include "highlite.h"

// Line::colorsFlags bitfield
#define LINE_STARTS_IN_COMMENT 0x01
#define LINE_ENDS_IN_COMMENT 0x02

struct Line
{
  bool ownsPtr;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  unsigned char colorFlags;
#endif
  line_xpos length;
  line_xpos capacity;
  char huge *text;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  unsigned char huge *colors;
#endif
  Line far *prevLine;
  Line far *nextLine;
};

Line far *advance_n_lines(Line far *line, line_num numLinesForward);
Line far *get_nth_line_of_document(line_num lineNumber); // Linear search from start of doc to find Nth line
Line far *get_nth_line_on_screen(cursor_pos screenY); // Linear search from start of screen to find Nth line

cursor_pos screen_end_y_of_last_line_on_screen(Line far *startingLine, cursor_pos startingScreenY);

// Returns 1-based line number
line_num get_line_number(Line far *line);
line_num get_current_line_number();

void delete_line(Line far *line);
bool increase_capacity(Line far *line, line_xpos newCapacity);

bool line_comes_before(Line far *a, Line far *b);

// Returns number of lines from a to b, or -1 if a does not precede b.
int16_t calculate_line_delta(Line far *a, Line far *b);

char last_nonws_char_on_line(Line far *line, line_xpos fromPosition);

line_xpos find_line_indent_position(Line far *line);

// Compares whether line1:col1 comes before or after line2:col2.
// Returns -1 if before, +1 if after and 0 if they are equal.
int8_t cursor_pos_cmp(line_num line1, line_xpos col1, line_num line2, line_xpos col2);

bool is_alphanum(char ch);
bool is_whitespace(char ch);
bool is_tab(char ch);

#endif
