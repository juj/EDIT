#include "line.h"
#include "screen.h"
#include "util.h"
#include "state.h"
#include "file.h"
#include "memory.h"
#include "iassert.h"

int find_line_indent_position(Line far *line)
{
  assert(line);
  // Find line indentation position
  int indentPos = 0;
  for(long i = 0; i < line->length && line->text[i] <= 32; ++i)
    ++indentPos;
  return indentPos;
}

Line far *advance_n_lines(Line far *line, long numLinesForward)
{
  assert(line);
  while(line->nextLine && numLinesForward-- > 0)
    line = line->nextLine;
  return line;
}

Line far *get_nth_line_of_document(long lineNumber)
{
  assert(currentFile);
  assert(currentFile->firstLine);
  return advance_n_lines(currentFile->firstLine, lineNumber - 1);
}

Line far *get_nth_line_on_screen(long screenY)
{
  assert(firstDisplayedLine);
  return advance_n_lines(firstDisplayedLine, screenY);
}

long screen_end_y_of_last_line_on_screen(Line far *startingLine, int startingScreenY)
{
  while(startingLine->nextLine && startingScreenY < SCREEN_HEIGHT)
  {
    ++startingScreenY;
    startingLine = startingLine->nextLine;
  }
  return startingScreenY;
}

long get_line_number(Line far *line)
{
  assert(line);
  long curLineNumber = 1;
  while(line->prevLine)
  {
    line = line->prevLine;
    ++curLineNumber;
  }
  return curLineNumber;
}

long get_current_line_number()
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

int increase_capacity(Line far *line, long newCapacity)
{
  assert(line);
  if (line->capacity >= newCapacity)
    return 1;

  char huge *newText = (char huge*)Farmalloc(newCapacity);
  if (!newText)
    return 0;
  for(long i = 0; i < line->length; ++i)
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

char last_nonws_char_on_line(Line far *line, long fromPosition)
{
  assert(line);
  for(long i = MIN(fromPosition, line->length-1); i >= 0; --i)
    if (line->text[i] > 32 && line->text[i] != CHAR_MIDDLE_DOT && line->text[i] != CHAR_RIGHT_ARROW)
      return line->text[i];
  return 0;
}

int calculate_line_delta(Line far *a, Line far *b)
{
  assert(a);
  assert(b);
  int delta = 0;
  while(a)
  {
    if (a == b)
      return delta;
    a = a->nextLine;
    ++delta;
  }
  return -1;
}

int line_comes_before(Line far *a, Line far *b)
{
  assert(a);
  assert(b);
  return calculate_line_delta(a, b) >= 0;
}

int cursor_pos_cmp(unsigned long line1, unsigned long col1, unsigned long line2, unsigned long col2)
{
  if (line1 < line2)
    return -1;
  if (line1 > line2)
    return 1;
  if (col1 < col2)
    return -1;
  if (col1 > col2)
    return 1;
  return 0;
}

