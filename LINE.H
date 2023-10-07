#ifndef _LINE_H
#define _LINE_H

// Line::colorsFlags bitfield
#define LINE_STARTS_IN_COMMENT 0x01
#define LINE_ENDS_IN_COMMENT 0x02

struct Line
{
  int ownsPtr;
  char huge *text;
  long length;
  long capacity;
  char far *colors;
  unsigned char colorFlags;
  Line far *prevLine;
  Line far *nextLine;
};

Line far *advance_n_lines(Line far *startingLine, long numLinesForward);
Line far *get_nth_line(long lineNumber);

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

int cursor_pos_cmp(unsigned long line1, unsigned long col1, unsigned long line2, unsigned long col2);

#endif
