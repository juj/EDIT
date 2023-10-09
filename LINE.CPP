#include "line.h"
#include "screen.h"
#include "util.h"
#include "state.h"
#include "file.h"
#include "memory.h"
#include "iassert.h"

line_xpos find_line_indent_position(Line far *line)
{
  assert(line);
  // Find line indentation position
  line_xpos indentPos = 0;
  for(line_xpos i = 0; i < line->length && is_whitespace(line->text[i]); ++i)
    ++indentPos;
  return indentPos;
}

Line far *advance_n_lines(Line far *line, line_num numLinesForward)
{
  assert(line);
  while(line->nextLine && numLinesForward-- > 0)
    line = line->nextLine;
  return line;
}

Line far *get_nth_line_of_document(line_num lineNumber)
{
  assert(currentFile);
  assert(currentFile->firstLine);
  return advance_n_lines(currentFile->firstLine, lineNumber - 1);
}

Line far *get_nth_line_on_screen(cursor_pos screenY)
{
  assert(firstDisplayedLine);
  Line *line = firstDisplayedLine;
  while(line && screenY-- > 0)
    line = line->nextLine;
  return line;

//  return advance_n_lines(firstDisplayedLine, screenY);
}

cursor_pos screen_end_y_of_last_line_on_screen(Line far *startingLine, cursor_pos startingScreenY)
{
  while(startingLine->nextLine && startingScreenY < SCREEN_HEIGHT)
  {
    ++startingScreenY;
    startingLine = startingLine->nextLine;
  }
  return startingScreenY;
}

line_num get_line_number(Line far *line)
{
  assert(line);
  line_num curLineNumber = 1;
  while(line->prevLine)
  {
    line = line->prevLine;
    ++curLineNumber;
  }
  return curLineNumber;
}

line_num get_current_line_number()
{
  assert(currentLine);
  return get_line_number(currentLine);
}

void delete_line(Line far *line)
{
  assert(line);
  if (line->ownsPtr)
    Farfree(line->text, line->capacity);
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  if (line->colors)
    Farfree(line->colors, line->capacity);
#endif
  Farfree(line, sizeof(Line));
}

bool increase_capacity(Line far *line, line_xpos newCapacity)
{
  assert(line);
  if (line->capacity >= newCapacity)
    return 1;

  char huge *newText = (char huge*)Farmalloc(newCapacity);
  if (!newText)
    return 0;
  for(line_xpos i = 0; i < line->length; ++i)
    newText[i] = line->text[i];
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  if (line->colors)
  {
    Farfree(line->colors, line->capacity);
    line->colors = 0;
  }
#endif
  if (line->ownsPtr)
    Farfree(line->text, line->capacity);
  else
  {
    // TODO: Move this ptr to small allocs buffer
  }
  line->text = newText;
  line->capacity = newCapacity;
  line->ownsPtr = 1;
  return 1;
}

char last_nonws_char_on_line(Line far *line, line_xpos fromPosition)
{
  assert(line);
  for(line_xpos i = MIN(fromPosition+1, line->length); i > 0;)
    if (!is_whitespace(line->text[--i]))
      return line->text[i];
  return 0;
}

int16_t calculate_line_delta(Line far *a, Line far *b)
{
  assert(a);
  assert(b);
  int16_t delta = 0;
  while(a)
  {
    if (a == b)
      return delta;
    a = a->nextLine;
    ++delta;
  }
  return -1;
}

bool line_comes_before(Line far *a, Line far *b)
{
  assert(a);
  assert(b);
  return calculate_line_delta(a, b) >= 0;
}

int8_t cursor_pos_cmp(line_num line1, line_xpos col1, line_num line2, line_xpos col2)
{
  if (line1 < line2) return -1;
  if (line1 > line2) return 1;
  if (col1 < col2) return -1;
  if (col1 > col2) return 1;
  return 0;
}

bool is_whitespace(char ch)
{
  return ch <= 32 || ch == CHAR_MIDDLE_DOT || ch == CHAR_RIGHT_ARROW;
}

bool is_tab(char ch)
{
  return ch == '\t' || ch == CHAR_RIGHT_ARROW;
}
