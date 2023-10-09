// SMINDENT.CPP: Smart Indent feature

#include "smindent.h"
#include "line.h"
#include "util.h"
#include "state.h"
#include "screen.h"
#include "audio.h"
#include "file.h"
#include "iassert.h"

int is_tab(char ch)
{
  return ch == '\t' || ch == CHAR_RIGHT_ARROW;
}

int is_space(char ch)
{
// TODO: maybe?
//  return (unsigned char)ch <= 32 || ch == (char)CHAR_MIDDLE_DOT || (unsigned char)ch == 0xBA || (unsigned char)ch == 0xFA;
  return ch == ' ' || ch == (char)CHAR_MIDDLE_DOT;
}

int gcd(int a, int b)
{
  int c;
  if (a < b) c = a, a = b, b = c;
  while(b > 0)
  {
    while(a >= b) a -= b;
    c = a, a = b, b = c;
  }
  return a;
}

int guess_space_indent_amount(int prevLine, int curLine)
{
  int indent = gcd(prevLine, curLine);
  if (indent > 1)
    return indent;

  indent = gcd(MIN(prevLine, curLine), ABS(prevLine-curLine));
  if (indent > 1)
    return indent;
  return indentSize;
}

int infer_indent_gcd(Line far *line)
{
  int numHistory = SCREEN_HEIGHT;
  int indent = find_line_indent_position(line);
  while(numHistory-- && indent > 1)
  {
     Line far *prev = line->prevLine;
     if (!prev)
       break;
     int prevIndent = find_line_indent_position(prev);
     if (prevIndent > 0)
     {
       indent = indent > 0 ? gcd(indent, prevIndent) : prevIndent;
     }
     line = prev;
  }
  return indent ? indent : indentSize;
}

void insert_character(char ch);
void repaint_screen();

void insert_indent()
{
  assert(currentLine);

  long currentCharOffset = current_column();

  // Guess indentation from previous line
  char guessedIndentChar = 0;
  int guessedIndentAmount = 1;

  Line far *prev = currentLine->prevLine;
  int prevIndentAmount = prev ? find_line_indent_position(prev) : 0;
  int curIndentAmount = find_line_indent_position(currentLine);

  if (currentCharOffset <= curIndentAmount)
  {
    if (prev && currentCharOffset < prevIndentAmount)
    {
      if (is_tab(prev->text[currentCharOffset]))
        guessedIndentChar = prev->text[currentCharOffset];
      else
      {
        guessedIndentChar = prev->text[currentCharOffset];
        guessedIndentAmount = infer_indent_gcd(prev);
//      guessedIndentAmount = guess_space_indent_amount(prevIndentAmount, MIN(currentCharOffset, curIndentAmount));
      }
    }
    else if (curIndentAmount > 0 && prev)
    {
      guessedIndentChar = currentLine->text[curIndentAmount-1];
      if (is_space(guessedIndentChar))
      {
        guessedIndentAmount = infer_indent_gcd(prev);
        /*
        if (prevIndentAmount > 0)
          guessedIndentAmount = guess_space_indent_amount(prevIndentAmount, curIndentAmount);
        else
          guessedIndentAmount = indentSize;
        */
      }
    }
    else
    {
      guessedIndentChar = tabChar;
      guessedIndentAmount = indentSize; // TODO: should be 1 for tabs mode?
    }
  }
  else
  {
    guessedIndentAmount = indentSize;
    // Find the last whitespace on the current line.
    for(long i = currentCharOffset-1; i >= 0; --i)
    {
      assert(i < currentLine->length);
      if (is_tab(currentLine->text[i]) || is_space(currentLine->text[i]))
      {
        guessedIndentChar = currentLine->text[i];
        break;
      }
    }
    if (!guessedIndentChar) guessedIndentChar = tabChar;
  }
  assert(is_tab(guessedIndentChar) || is_space(guessedIndentChar));
  for(int i = 0; i < guessedIndentAmount; ++i)
    insert_character(guessedIndentChar);
}

void repaint_current_line_from_offset(int xOffset, int xEnd);
void verify_screen_is_painted_correctly();

void unindent()
{
  assert(currentLine);
  int curIndentAmount = find_line_indent_position(currentLine);

  if (curIndentAmount == 0)
  {
    ERROR_BEEP();
    return;
  }

  // Find the most recent line with less indentation, and align
  // to that line
  Line far *prev = currentLine->prevLine;
  int numCharsToUnindent = 0;

  if (!prev)
  {
    numCharsToUnindent = MIN(curIndentAmount, indentSize);
  }
  while(prev)
  {
    int prevIndentAmount = find_line_indent_position(prev);
    if (prevIndentAmount < curIndentAmount)
    {
      numCharsToUnindent = curIndentAmount - prevIndentAmount;
      break;
    }
    prev = prev->prevLine;
  }
  // Or indent to next line, if that is shorter
  Line far *next = currentLine->nextLine;
  if (next)
  {
    int nextIndentAmount = find_line_indent_position(next);
    if (curIndentAmount > nextIndentAmount)
      numCharsToUnindent = MIN(numCharsToUnindent, curIndentAmount - nextIndentAmount);
  }

  if (numCharsToUnindent == 0)
  {
    ERROR_BEEP();
    return;
  }

  curIndentAmount -= numCharsToUnindent;

  for(long i = curIndentAmount; i + numCharsToUnindent < currentLine->length; ++i)
    currentLine->text[i] = currentLine->text[i + numCharsToUnindent];
  currentLine->length -= numCharsToUnindent;
  clear_highlight(currentLine);
  mark_current_file_modified();

  int oldCursorPos = cursorX + viewportXOffset;
  int newCursorPos = MAX(oldCursorPos - numCharsToUnindent, curIndentAmount);
  viewportXOffset = MIN(viewportXOffset, curIndentAmount);
  cursorX = virtualX = newCursorPos - viewportXOffset;

  repaint_current_line_from_offset(cursorX, currentLine->length + numCharsToUnindent);
  verify_screen_is_painted_correctly();
}

bool all_chars_left_of_cursor_are_whitespace()
{
  int x = current_column();
  for(int i = 0; i < x; ++i)
    if (!is_whitespace(currentLine->text[i]))
      return false;
  return true;
}
