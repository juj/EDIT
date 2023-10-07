#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "file.h"
#include "input.h"
#include "screen.h"
#include "state.h"
#include "audio.h"
#include "line.h"
#include "openfile.h"
#include "viewport.h"
#include "memory.h"
#include "copypast.h"
#include "highlite.h"
#include "cursor.h"
#include "smindent.h"

void repaint_open_files_list(File *highlightFilename)
{
  File *f = currentFile;
  while(f->prevFile)
    f = f->prevFile;
  int x = SCREEN_WIDTH;
  while(f)
  {
    char fname[16];
    assert(f->filename);
    sprintf(fname, f->hasUnsavedChanges ? "%s*" : "%s", f->filename);
    x -= (int)strlen(fname);
    unsigned char foreColor = (f->hasUnsavedChanges ? LTRED :
       (f == currentFile ? LTCYAN : LTGRAY));
    unsigned char bgColor = (f == highlightFilename) ? BGCOLOR(LTGRAY) : BGCOLOR(BLACK);

    put_string_on_screen(fname, x, 0, foreColor | bgColor);
    f = f->nextFile;
    if (f)
    {
      put_string_on_screen("|", --x, 0, LTGRAY);
    }
  }
}

void repaint_screen_lines(int yStart, int yEnd)
{
  assert(yStart >= 0);
  assert(yStart <= SCREEN_HEIGHT);
  assert(yStart <= yEnd);
  assert(yEnd >= 0);
  assert(yEnd <= SCREEN_HEIGHT);
  Line far *curLine = get_nth_line_on_screen(yStart);
  int y = 0;
  int lineNumber = firstDisplayedLineNumber + yStart;

  const unsigned char clearCHs[4] = { 0xB2, 0xB1, 0xB0, 0 };
  unsigned char clearCH = 0;

  for(; y < yEnd; ++y)
  {
    if (curLine)
    {
      syntax_highlight(curLine);

      int selStart = lineNumber == selectionStartLineNum ? selectionStartColumn : (lineNumber <= selectionEndLineNum ? 0 : 0x7FFF);
      int selEnd = lineNumber == selectionEndLineNum ? selectionEndColumn : (lineNumber >= selectionStartLineNum ? 0x7FFF : 0);
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
      if (curLine->colors) show_line_with_colors_with_sel(y * SCREEN_WIDTH, curLine->text, curLine->length, curLine->colors, selStart, selEnd);
      else
#endif
        show_line_with_sel(y * SCREEN_WIDTH, curLine->text, curLine->length, selStart, selEnd);

      if (y > 0 && y < SCREEN_HEIGHT-1)
      {
        unsigned char existingColor = get_screen_color(y*SCREEN_WIDTH+SCREEN_WIDTH-1);
        if (curLine->length - viewportXOffset > SCREEN_WIDTH)
          put_screen_color(y*SCREEN_WIDTH+SCREEN_WIDTH-1, existingColor | BGCOLOR(BROWN));
        else
          put_screen_color(y*SCREEN_WIDTH+SCREEN_WIDTH-1, existingColor & ~BGCOLOR(BROWN));
      }

      curLine = curLine->nextLine;
      ++lineNumber;
    }
    else
      clear_line(y * SCREEN_WIDTH, clearCHs[clearCH < 3 ? clearCH++ : 3]);

    if (viewportXOffset != 0)
      put_screen_color(y*SCREEN_WIDTH, get_screen_color(y*SCREEN_WIDTH) | BGCOLOR(BROWN));
  }
  if (yStart == 0)
    repaint_open_files_list(0);
}

void print_line_column_info()
{
  assert(get_current_line_number() == current_line_number());
  //  long curLineNumber = get_current_line_number();
  char str[64];
  /*
  sprintf(str, " %lub near/%lub far mem used    %ld:%d",
  allocatedNearMemory, allocatedFarMemory,
  curLineNumber, char_position_on_current_line()+1);
  */

  sprintf(str, " %ld:%ld", current_line_number(), current_column()+1);

  put_string_on_screen(str, SCREEN_WIDTH - (int)strlen(str), SCREEN_HEIGHT-1, LTGRAY);
}

void repaint_screen()
{
  repaint_screen_lines(0, SCREEN_HEIGHT);
  print_line_column_info();
  set_cursor_pos(cursorX, cursorY);
}

void verify_screen_is_painted_correctly()
{
  save_screen();
  repaint_screen();
  assert(saved_screen_matches_current_screen());
}

void repaint_current_line_from_offset(int xOffset)
{
  // TODO: actually paint from current offset onwards only
  repaint_screen_lines(cursorY, cursorY+1);
}

void insert_character(char ch)
{
  assert(currentLine);
  int currentCharOffset = char_position_on_current_line();

  if (insertMode && currentCharOffset < currentLine->length)
  {
    currentLine->text[currentCharOffset] = ch;
    clear_highlight(currentLine);
    FILE_MODIFIED();
    put_screen(cursorY*SCREEN_WIDTH+cursorX, ch);
    ++cursorX;
    virtualX = cursorX;
  }
  else
  {
    // If the current cursor position is outside the end of the line,
    // we may need to insert one or more whitespace as well to create
    // a properly indented text.
    int numCharsToInsert = 1 + MAX(0, currentCharOffset - currentLine->length);

    if (currentLine->length + numCharsToInsert > currentLine->capacity)
    {
      if (!increase_capacity(currentLine, currentLine->capacity*2+numCharsToInsert))
      return;
    }

    // Shift existing characters up
    for(long i = currentLine->length + numCharsToInsert - 1; i > currentCharOffset; --i)
    {
      assert(i < currentLine->capacity);
      assert(i-numCharsToInsert >= 0);
      assert(i-numCharsToInsert < currentLine->length);
      assert(currentLine->text);
      currentLine->text[i] = currentLine->text[i-numCharsToInsert];
    }

    // Insert indentation that matches the previous line
    Line far *prevLine = currentLine->prevLine;
    if (prevLine)
    {
      for(long j = 1; j < numCharsToInsert; ++j)
      {
        assert(currentCharOffset - j >= 0 && currentCharOffset - j < currentLine->length);
        assert(numCharsToInsert-1-j >= 0 && prevLine->length && numCharsToInsert-1-j < prevLine->length);

        assert(currentLine->text);
        currentLine->text[currentCharOffset - j] =
            (prevLine->text[numCharsToInsert-1-j] <= 32)
           ? prevLine->text[numCharsToInsert-1-j] : tabChar;
      }
    }

    assert(currentCharOffset >= 0);
    assert(currentCharOffset <= currentLine->length);
    assert(currentCharOffset < currentLine->capacity);
    assert(currentLine->text);
    currentLine->text[currentCharOffset] = ch;
    currentLine->length += numCharsToInsert;
    assert(currentLine->length <= currentLine->capacity);
    clear_highlight(currentLine);
    FILE_MODIFIED();
    int full_repaint = 0;
    if (++cursorX >= SCREEN_WIDTH)
    {
      --cursorX;
      ++viewportXOffset;
      full_repaint = 1;
    }
    virtualX = cursorX;

    // TODO: only repaint the current line starting from currentCharOffset
    repaint_screen();
    /*
    if (full_repaint) repaint_screen();
    else
    {
      repaint_current_line_from_offset(cursorX-1);
    }
    */
  }
}

void delete_character()
{
  assert(currentLine);
  int currentCharOffset = char_position_on_current_line();
  if (currentCharOffset < currentLine->length)
  {
    while(currentCharOffset+1 < currentLine->length)
    {
      currentLine->text[currentCharOffset] = currentLine->text[currentCharOffset+1];
      ++currentCharOffset;
    }
    --currentLine->length;
  }
  else // join next line on this
  {
    Line far *next = currentLine->nextLine;
    if (!currentLine->nextLine)
    {
      ERROR_BEEP();
      return;
    }
    increase_capacity(currentLine, currentLine->length + next->length);
    for(long i = 0; i < next->length; ++i)
      currentLine->text[currentLine->length + i] = next->text[i];
    currentLine->length += next->length;
    currentLine->nextLine = next->nextLine;
    if (currentLine->nextLine)
    {
      currentLine->nextLine->prevLine = currentLine;
    }
    else
    {
      assert(next == currentFile->lastLine);
      currentFile->lastLine = currentLine;
    }
    delete_line(next);
  }
  clear_highlight(currentLine);
  FILE_MODIFIED();
  // TODO: only repaint the current line starting from currentCharOffset
//  repaint_current_line_from_offset(cursorX-1);
  repaint_screen();
}

void unindent();

void backspace_character()
{
  int currentCharOffset = char_position_on_current_line();
  if (currentCharOffset > 0)
  {
    if (currentCharOffset == find_line_indent_position(currentLine))
    {
      unindent();
      return;
    }
    --cursorX;
    virtualX = cursorX;
    delete_character();
  }
  else if (currentLine->prevLine)
  {
    currentLine = currentLine->prevLine;
    --cursorY;
    viewportXOffset = 0; // TODO: Calculate this correctly.
    cursorX = virtualX = currentLine->length;
    delete_character();
  }
}

void restrict_cursor_on_line()
{
  cursorX = MIN(virtualX, currentLine->length - viewportXOffset);
  if (cursorX < 0)
  {
    viewportXOffset += cursorX;
    cursorX = 0;
    repaint_screen();
  }
  if (cursorX >= SCREEN_WIDTH)
  {
    viewportXOffset += cursorX - (SCREEN_WIDTH-1);
    cursorX = SCREEN_WIDTH - 1;
    repaint_screen();
  }
}

Line far *find_last_viewport_start_line(int *linesInView);
void scroll_to_beginning();
void scroll_to_end();
void restrict_cursor_on_line();

void move_viewport_vertically(int lines, int cursor_moves_along)
{
  int scrolled = 0;

  while(lines < 0)
  {
    if (firstDisplayedLine->prevLine)
    {
      ++lines;
      scrolled = 1;
      firstDisplayedLine = firstDisplayedLine->prevLine;
      --firstDisplayedLineNumber;
      if (cursor_moves_along || cursorY + 1 >= SCREEN_HEIGHT)
      {
        currentLine = currentLine->prevLine;
      }
      else
        ++cursorY;
      repaint_screen();
    }
    else
      break;
  }

  if (lines > 0)
  {
    Line far *lastPossibleViewportStartLine = find_last_viewport_start_line(0);

    while(lines > 0)
    {
      if (currentLine->nextLine && calculate_line_delta(firstDisplayedLine, lastPossibleViewportStartLine) > 0)
      {
        --lines;
        scrolled = 1;
        firstDisplayedLine = firstDisplayedLine->nextLine;
        ++firstDisplayedLineNumber;
        if (cursor_moves_along || cursorY <= 0)
        {
          currentLine = currentLine->nextLine;
        }
        else
          --cursorY;
        repaint_screen();
      }
      else
        break;
    }
  }

  if (!scrolled && cursor_moves_along)
  {
    if (lines < 0) scroll_to_beginning();
    else scroll_to_end();
  }

  restrict_cursor_on_line();
}

void move_cursor_vertically(int lines)
{
  int makeSound = 0;

  while(lines < 0)
  {
    assert(currentLine);
    if (cursorY > 0)
    {
      --cursorY;
      makeSound = -1;
      assert(currentLine->prevLine);
      currentLine = currentLine->prevLine;
    }
    else
    {
      assert(currentLine == firstDisplayedLine);
      if (currentLine->prevLine)
      {
        makeSound = -1;
        currentLine = firstDisplayedLine = currentLine->prevLine;
        --firstDisplayedLineNumber;
        repaint_screen();
      }
      else if (!makeSound)
        makeSound = 1;
    }
    ++lines;
  }

  while(lines > 0)
  {
    assert(currentLine);
    if (currentLine->nextLine)
    {
      currentLine = currentLine->nextLine;
      makeSound = -1;
      if (cursorY < SCREEN_HEIGHT-1)
      {
        ++cursorY;
      }
      else
      {
        firstDisplayedLine = firstDisplayedLine->nextLine;
        ++firstDisplayedLineNumber;
        repaint_screen();
      }
    }
    else if (!makeSound)
      makeSound = 1;
    --lines;
  }

  restrict_cursor_on_line();

  if (makeSound == 1)
    beep(170, 50);
}

void move_cursor_horizontally(int chars)
{
  int oldViewportXOffset = viewportXOffset;

  if (chars < 0)
  {
    int left = MIN(cursorX, -chars);
    cursorX -= left;
    virtualX = cursorX;
    chars += left;

    left = MIN(viewportXOffset, -chars);
    viewportXOffset -= left;
    chars += left;

    if (chars != 0)
    {
      if (currentLine->prevLine)
      {
  move_cursor_vertically(-1);
  cursorX = virtualX = currentLine->length;
  left = MAX(0, cursorX - (SCREEN_WIDTH-1));
  cursorX -= left;
  viewportXOffset += left;

  move_cursor_horizontally(chars+1);
      }
      else
  ERROR_BEEP();
    }
  }
  else if (chars > 0)
  {
    int right = MIN(currentLine->length - viewportXOffset - cursorX, chars);
    cursorX += right;
    virtualX = cursorX;
    chars -= right;

    right = MAX(0, cursorX - (SCREEN_WIDTH-1));
    cursorX -= right;
    viewportXOffset += right;

    if (chars != 0)
    {
      if (currentLine->nextLine)
      {
        move_cursor_vertically(1);
        cursorX = virtualX = 0;
        viewportXOffset = 0;
        move_cursor_horizontally(chars-1);
      }
      else
        ERROR_BEEP();
    }
  }
  if (oldViewportXOffset != viewportXOffset)
    repaint_screen();
}

void toggle_cursor_home()
{
  int indentPos = find_line_indent_position(currentLine);
  cursorX = virtualX = (cursorX != indentPos) ? indentPos : 0;

  int oldViewportXOffset = viewportXOffset;
  viewportXOffset = MAX(0, cursorX - SCREEN_WIDTH);
  if (oldViewportXOffset != viewportXOffset)
    repaint_screen();
}

Line far *find_last_viewport_start_line(int *linesInView)
{
  Line far *lastLine = currentFile->lastLine;
  int y = 0;
  for(;y < SCREEN_HEIGHT - 2 && lastLine->prevLine; ++y)
    lastLine = lastLine->prevLine;
  if (linesInView) *linesInView = y;
  return lastLine;
}

void scroll_to_beginning()
{
  currentLine = firstDisplayedLine = currentFile->firstLine;
  firstDisplayedLineNumber = 1;
  cursorX = virtualX = 0;
  cursorY = 0;
  viewportXOffset = 0;
  repaint_screen();
}

void scroll_to_end()
{
  Line far *lastLine = currentFile->lastLine;
  assert(!lastLine->nextLine);
  // Cursor will be on last line of file
  currentLine = lastLine;
  // Viewport will start one screenful earlier
  firstDisplayedLine = find_last_viewport_start_line(&cursorY);
  firstDisplayedLineNumber = get_line_number(firstDisplayedLine); // TODO: This and above line can be fused to one common loop.
  int minimumViewportX = currentLine->length - (SCREEN_WIDTH-1);
  viewportXOffset = MAX(viewportXOffset, minimumViewportX);
  cursorX = virtualX = currentLine->length - viewportXOffset;
  repaint_screen();
}

void move_cursor_to_line_end()
{
  int minimumXOffset = MAX(0, currentLine->length - (SCREEN_WIDTH-1));
  int oldOffset = viewportXOffset;
  viewportXOffset = MAX(viewportXOffset, minimumXOffset);
  cursorX = virtualX = currentLine->length - viewportXOffset;
  if (viewportXOffset != oldOffset)
    repaint_screen();
}

void move_cursor_to_prev_word()
{
  Line far *line = currentLine;
  long currentCharOffset = char_position_on_current_line()-1;

  for(; currentCharOffset >= 0; --currentCharOffset)
    if (isalnum((unsigned char)line->text[currentCharOffset])
  && (currentCharOffset == 0 || !isalnum((unsigned char)line->text[currentCharOffset-1])))
    break;

  if (currentCharOffset < 0)
  {
    if (!line->prevLine)
    {
      ERROR_BEEP();
      return;
    }
    currentLine = line->prevLine;
    if (cursorY > 0)
      --cursorY;
    else
    {
      firstDisplayedLine = firstDisplayedLine->prevLine;
      --firstDisplayedLineNumber;
    }
    cursorX = virtualX = currentLine->length;
  }
  else
  {
    long oldXOffset = viewportXOffset;
    viewportXOffset = MIN(viewportXOffset, currentCharOffset);
    cursorX = virtualX = currentCharOffset - viewportXOffset;
    if (oldXOffset != viewportXOffset)
      repaint_screen();
  }
}

void move_cursor_to_next_word()
{
  Line far *line = currentLine;
  long currentCharOffset = char_position_on_current_line()+1;

  for(; currentCharOffset < line->length; ++currentCharOffset)
    if (isalnum((unsigned char)line->text[currentCharOffset])
  && !isalnum((unsigned char)line->text[currentCharOffset-1]))
    break;

  if (currentCharOffset >= line->length)
  {
    if (!line->nextLine)
    {
      ERROR_BEEP();
      return;
    }
    currentLine = line->nextLine;
    if (cursorY < SCREEN_HEIGHT-1)
      ++cursorY;
    else
    {
      firstDisplayedLine = firstDisplayedLine->nextLine;
      ++firstDisplayedLineNumber;
    }
    cursorX = virtualX = find_line_indent_position(currentLine);
    viewportXOffset = 0;
  }
  else
  {
    long oldXOffset = viewportXOffset;
    viewportXOffset = MIN(viewportXOffset, currentCharOffset);
    cursorX = virtualX = currentCharOffset - viewportXOffset;
    if (oldXOffset != viewportXOffset)
      repaint_screen();
  }
}

void insert_new_line(int cursorMovesToNewline)
{
  long currentCharOffset = char_position_on_current_line();

  // TODO: Also guess indent here
  long newIndent = (last_nonws_char_on_line(currentLine, currentCharOffset-1) == '{') ? indentSize : 0;

  // We copy indent from current line to the new line up to
  // the character position
  long curLineIndent = MIN(currentCharOffset, find_line_indent_position(currentLine));

  Line far *newLine = (Line far *)Farmalloc(sizeof(Line));
  if (!newLine)
    return;

  // move all chars from current line cursor position onwards to new line

  // TODO: With a smart shared pointer handling of the char data,
  // could split here with a no-alloc scheme.

  newLine->length = newLine->capacity = curLineIndent + newIndent + currentLine->length - currentCharOffset;
  newLine->text = newLine->length > 0 ? (char huge *)Farmalloc(newLine->length) : 0;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  newLine->colors = 0;
#endif
  if (!newLine->text && newLine->length > 0)
  {
    delete_line(newLine);
    return;
  }
  // Copy indent
  for(long i = 0; i < curLineIndent; ++i)
    newLine->text[i] = currentLine->text[i];
  for(long j = 0; j < newIndent; ++j)
    newLine->text[curLineIndent+j] = tabChar;

  // Copy leftover text from the current line to the new line
  for(long k = 0; k < newLine->length && currentCharOffset+k < currentLine->length; ++k)
    newLine->text[curLineIndent+newIndent+k] = currentLine->text[currentCharOffset+k];
  currentLine->length -= currentLine->length - currentCharOffset;
  newLine->ownsPtr = 1;
  newLine->prevLine = currentLine;
  newLine->nextLine = currentLine->nextLine;
  if (currentLine->nextLine)
  {
    currentLine->nextLine->prevLine = newLine;
  }
  else
  {
    // Update last line in file, if we added a new line at the end of file.
    assert(currentLine == currentFile->lastLine);
    currentFile->lastLine = newLine;
  }

  currentLine->nextLine = newLine;
  clear_highlight(currentLine);

  if (cursorMovesToNewline)
  {
    // MIN(cursorX, find_line_indent_position(currentLine));
    cursorX = virtualX = curLineIndent + newIndent;//find_line_indent_position(currentLine);
    if (cursorY < SCREEN_HEIGHT-1)
      ++cursorY;
    else
    {
      firstDisplayedLine = firstDisplayedLine->nextLine;
      ++firstDisplayedLineNumber;
    }
    viewportXOffset = 0;
    currentLine = newLine;
  }

  FILE_MODIFIED();

  // TODO: Hoist this higher
  repaint_screen();
}

void erase_line(Line *line)
{
  if (!line)
  {
    ERROR_BEEP();
    return;
  }
  assert(currentFile);
  if (line == currentFile->firstLine && line == currentFile->lastLine)
  {
    if (line->length > 0)
    {
      line->length = 0;
      currentFile->hasUnsavedChanges = 1;
    }
    return;
  }

  currentFile->hasUnsavedChanges = 1;

  Line far *subst = line->nextLine ? line->nextLine : line->prevLine;
  if (line == firstDisplayedLine)
  {
    firstDisplayedLine = subst;
    --firstDisplayedLineNumber;
  }
  if (line == currentLine)
  {
    if (subst == line->prevLine) --cursorY;
    currentLine = subst;
  }

  if (line == currentFile->firstLine) currentFile->firstLine = line->nextLine;
  if (line == currentFile->lastLine) currentFile->lastLine = line->prevLine;

  if (line->prevLine) line->prevLine->nextLine = line->nextLine;
  if (line->nextLine) line->nextLine->prevLine = line->prevLine;
  delete_line(line);
}

void run_goto_line_menu()
{
  long origLineNumber = get_current_line_number();
  char targetLineNumberStr[32] = {0};

  if (viewportXOffset > 0)
  {
    viewportXOffset = 0;
    repaint_screen();
  }

  put_string_on_screen("Go to line: ", 0, SCREEN_HEIGHT-1, LTGRAY);
  char origLineNumberStr[32];
  sprintf(origLineNumberStr, "%ld ", origLineNumber);
  put_string_on_screen(origLineNumberStr, 12, SCREEN_HEIGHT-1, DKGRAY);
  set_cursor_pos(12, SCREEN_HEIGHT-1);
  const char *empty = " ";

//  Line far *originalFirstDisplayedLine = firstDisplayedLine;
  Line far *originalCurrentLine = currentLine;
  Line far *originalFirstDisplayedLine = firstDisplayedLine;
  long originalCursorY = cursorY;
  long originalViewportPos = get_line_number(firstDisplayedLine);
  long viewportPos = originalViewportPos;
  highlight_line(origLineNumber - originalViewportPos, BGCOLOR(LTGRAY));
  long inputLen = 0;
  long documentLengthLines = get_line_number(currentFile->lastLine);
  while(1)
  {
    int key = get_key();
    if ((key == KEY_BACKSPACE && inputLen > 0) || (key >= '0' && key <= '9' && inputLen < sizeof(targetLineNumberStr)-1
     && (inputLen > 0 || key != '0')))
    {
      if (key == KEY_BACKSPACE)
        targetLineNumberStr[--inputLen] = 0;
      else if (inputLen < 8)
        targetLineNumberStr[inputLen++] = (char)key;

      long targetLineNumber = inputLen == 0 ? origLineNumber : MIN(documentLengthLines, atol(targetLineNumberStr));

      if (targetLineNumber < viewportPos || targetLineNumber >= viewportPos + SCREEN_HEIGHT-1)
      {
        viewportPos = MAX(1, targetLineNumber - SCREEN_HEIGHT/2);
        firstDisplayedLine = get_nth_line_of_document(viewportPos);
        firstDisplayedLineNumber = viewportPos;
      }
      currentLine = get_nth_line_of_document(targetLineNumber);
      cursorY = targetLineNumber - viewportPos;
      repaint_screen();
      put_string_on_screen("Go to line: ", 0, SCREEN_HEIGHT-1, LTGRAY);
      assert(targetLineNumber >= 1);
      assert(viewportPos >= 1);
      assert(targetLineNumber - viewportPos >= 0);
      assert(targetLineNumber - viewportPos < SCREEN_HEIGHT);
      highlight_line(targetLineNumber - viewportPos, BGCOLOR(LTGRAY));

      put_string_on_screen(targetLineNumberStr, 12, SCREEN_HEIGHT-1, YELLOW);
      put_string_on_screen(empty, 12 + inputLen, SCREEN_HEIGHT-1, YELLOW);

      set_cursor_pos(12 + inputLen, SCREEN_HEIGHT-1);
      if (inputLen == 0)
      {
        put_string_on_screen(origLineNumberStr, 12, SCREEN_HEIGHT-1, DKGRAY);
        set_cursor_pos(12, SCREEN_HEIGHT-1);
      }
      // TODO: Goto line
    }
    else if (key == KEY_ESC)
    {
      currentLine = originalCurrentLine;
      firstDisplayedLine = originalFirstDisplayedLine;
      firstDisplayedLineNumber = originalViewportPos;
      cursorY = originalCursorY;
      break;
    }
    else if (key == KEY_ENTER)
    {
      cursorX = virtualX = find_line_indent_position(currentLine);
      break;
    }
  }
  // TODO: Only need to repaint last line.
  repaint_screen();
}

void erase_current_line()
{
  erase_line(currentLine);
  Line far *lastPossibleViewportStartLine = find_last_viewport_start_line(0);
  int line_delta = calculate_line_delta(lastPossibleViewportStartLine, firstDisplayedLine);
  if (line_delta > 0)
  {
    firstDisplayedLine = lastPossibleViewportStartLine;
    --firstDisplayedLineNumber;
    cursorY += line_delta;
  }
  repaint_screen();
}

//extern unsigned short pendingScanCode; // TODO REMOVE

void print_textsel_info()
{
  // TODO: Can this be optimized to occur less frequently?
  repaint_screen_lines(SCREEN_HEIGHT-1, SCREEN_HEIGHT);
  if (!selectionStartLine)
    return;

  assert(selectionStartLineNum <= selectionEndLineNum);
  unsigned long numLinesSelected;
  unsigned long numCharsSelected;
  calculate_num_lines_and_chars_selected(&numLinesSelected, &numCharsSelected);

  char str[64];
  if (numCharsSelected == 0)
    return;
  if (numLinesSelected == 1)
    sprintf(str, "%lu chars selected.", numCharsSelected);
  else
    sprintf(str, "%lu lines, %lu chars selected.", numLinesSelected, numCharsSelected);
  put_string_on_screen(str, (SCREEN_WIDTH-(int)strlen(str))>>1, SCREEN_HEIGHT-1, LTMAGENTA);
}

void select_all()
{
  scroll_to_end();
  selectionStartLine = currentFile->firstLine;
  selectionStartLineNum = 1;
  selectionStartColumn = 0;
  selectionEndLine = currentLine;
  selectionEndLineNum = current_line_number();
  selectionEndColumn = current_column();
  repaint_screen();
  print_textsel_info();
}

void main_loop()
{
  clear_screen(' ', editorTextColor);
  repaint_screen();
  int running = 1;

  int oldCursorPos;
  unsigned long oldLineNum;
  Line far *oldLine;

  while(running)
  {
    print_line_column_info();
    set_cursor_pos(cursorX, cursorY);

    assert(firstDisplayedLine);
    assert(get_line_number(firstDisplayedLine) == firstDisplayedLineNumber);
    assert(currentLine);
    assert(get_line_number(currentLine) == current_line_number());
//    verify_screen_is_painted_correctly();
    int key = get_key();

    // Doing text selection?
    int textSelection = 0;
    if (is_navigation_action_key(key))
    {
      textSelection = is_shift_key_held(key);
      if (textSelection)
      {
        oldCursorPos = current_column();
        oldLineNum = current_line_number();
        oldLine = currentLine;
        if (!selectionStartLine)
        {
          selectionStartLine = selectionEndLine = oldLine;
          selectionStartLineNum = selectionEndLineNum = oldLineNum;
          selectionStartColumn = selectionEndColumn = oldCursorPos;
        }
      }
      else if (selectionStartLine) // clear any selection
      {
        int startLine = selectionStartLineNum-firstDisplayedLineNumber;
        startLine = MAX(0, startLine);
        int endLine = selectionEndLineNum+1-firstDisplayedLineNumber;
        endLine = MIN(SCREEN_HEIGHT, endLine);
        clear_selection();
        repaint_screen_lines(startLine, endLine);
        repaint_screen_lines(SCREEN_HEIGHT-1, SCREEN_HEIGHT);
      }
    }

    switch(key)
    {
      case KEY_LEFT: move_cursor_horizontally(-1); break;
      case KEY_RIGHT: move_cursor_horizontally(1); break;
      case KEY_UP: move_cursor_vertically(-1); break;
      case KEY_DOWN: move_cursor_vertically(1); break;
      case KEY_PGUP: move_viewport_vertically(-SCREEN_HEIGHT, 1); break;
      case KEY_PGDN: move_viewport_vertically(SCREEN_HEIGHT, 1); break;
      case KEY_HOME: toggle_cursor_home(); break;
      case KEY_END: move_cursor_to_line_end(); break;
      case KEY_CTRL_LEFT: move_cursor_to_prev_word(); break;
      case KEY_CTRL_RIGHT: move_cursor_to_next_word(); break;
      case KEY_CTRL_UP: move_viewport_vertically(-1, 0); break;
      case KEY_CTRL_DOWN: move_viewport_vertically(1, 0); break;
      case KEY_CTRL_HOME: scroll_to_beginning(); break;
      case KEY_CTRL_END: scroll_to_end(); break;
      case KEY_ESC:
      case KEY_ALT_F4:
      case KEY_CTRL_Q:
        running = 0;
        break;
      case KEY_CTRL_A: select_all(); break;
      case KEY_BACKSPACE:
        if (selectionStartLine)
          erase_selected_text();
        else
          backspace_character();
        break;
      case KEY_SHIFT_DELETE:
        cut_selection();
        break;
      case KEY_DELETE:
        if (selectionStartLine)
        {
          erase_selected_text();
          repaint_screen(); // TODO better repaint
        }
        else
          delete_character();
        break;
      case KEY_CTRL_DELETE:
        if (selectionStartLine)
          erase_selected_text();
        else
          erase_current_line();
        break;
      case KEY_CTRL_K:
        erase_current_line(); break; // TODO: optimize to repaint only from given line forward
      case KEY_SHIFT_INSERT:
        paste_selection(); // TODO: this does not work, Shift-Ins generates a zero instead.
        break;
      case KEY_INSERT:
        toggle_insert();
        break;
      case KEY_ENTER: insert_new_line(/*cursorMovesToNewLine=*/1); break;
      case KEY_TAB: insert_indent(); break;
      case KEY_SHIFT_TAB: unindent(); break;
      case KEY_CTRL_TAB:
      case KEY_ALT_TAB: run_alt_tab_menu(); repaint_screen(); break;
      case KEY_CTRL_N:
      {
        File *f = open_new_file();
        if (f)
          open_file_in_viewport(f, /*bringToMruFront*/1);
      }
        break;
      case KEY_CTRL_C:
      case KEY_CTRL_INSERT:
        copy_selection();
        break;
      case KEY_CTRL_X:
        cut_selection();
        break;
      case KEY_CTRL_V:
        paste_selection();
        break;
      case KEY_CTRL_S:
      case KEY_F2:
        save_file(currentFile);
        repaint_screen();
        break;
      case KEY_CTRL_G: run_goto_line_menu(); break;
      case KEY_CTRL_O:
      case KEY_F3:
        run_open_file_menu();
        repaint_screen();
        break;
      case KEY_CTRL_W:
      case KEY_CTRL_F4:
        close_file(currentFile);
        repaint_screen();
        break;

#if defined(__TURBOC__) && defined(__MSDOS__)
      case KEY_CTRL_PLUS:
        if (currentVideoMode > 0)
        {
          set_vga_text_mode(--currentVideoMode);
          while(cursorY >= SCREEN_HEIGHT)
          {
            --cursorY;
            assert(currentLine->prevLine);
            currentLine = currentLine->prevLine;
          }
          repaint_screen();
        }
        break;
      case KEY_CTRL_MINUS:
        if (currentVideoMode < LAST_VIDEO_MODE)
        {
          set_vga_text_mode(++currentVideoMode);
          repaint_screen();
        }
        break;
      case KEY_F10:
        clear_screen(0, LTGRAY);
        clrscr();
        system("command.com");
        repaint_screen();
        break;
#endif

      default:
        if (key >= 0x20 && key < 127)
        {
          if (showWhitespace && key == ' ') key = CHAR_MIDDLE_DOT;
          insert_character(key);
        }
        else
        {
          set_cursor_pos(0,0);
          printf("0x%x     \n", key);
        }
        break;
    }
    if (textSelection)
    {
      int cursorPos = current_column();
      unsigned long lineNum = current_line_number();
      Line far *line = currentLine;
      int oldCmpStart = cursor_pos_cmp(oldLineNum, oldCursorPos, selectionStartLineNum, selectionStartColumn);
      if (!oldCmpStart)
      {
        int newCmpEnd = cursor_pos_cmp(lineNum, cursorPos, selectionEndLineNum, selectionEndColumn);
        if (newCmpEnd <= 0)
        {
          selectionStartLineNum = lineNum;
          selectionStartLine = line;
          selectionStartColumn = cursorPos;
        }
        else
        {
          selectionStartLineNum = selectionEndLineNum;
          selectionStartLine = selectionEndLine;
          selectionStartColumn = selectionEndColumn;
          selectionEndLineNum = lineNum;
          selectionEndLine = line;
          selectionEndColumn = cursorPos;
        }
      }
      else
      {
        assert(!cursor_pos_cmp(oldLineNum, oldCursorPos, selectionEndLineNum, selectionEndColumn));
        int startCmpNew = cursor_pos_cmp(selectionStartLineNum, selectionStartColumn, lineNum, cursorPos);
        if (startCmpNew <= 0)
        {
          selectionEndLineNum = lineNum;
          selectionEndLine = line;
          selectionEndColumn = cursorPos;
        }
        else
        {
          selectionEndLineNum = selectionStartLineNum;
          selectionEndLine = selectionStartLine;
          selectionEndColumn = selectionStartColumn;
          selectionStartLineNum = lineNum;
          selectionStartLine = line;
          selectionStartColumn = cursorPos;
        }
      }
      int repaintY0 = MIN(oldLineNum, lineNum) - firstDisplayedLineNumber;
      repaintY0 = MAX(0, repaintY0);
      int repaintY1 = MAX(oldLineNum, lineNum) + 1 - firstDisplayedLineNumber;
      repaintY1 = MIN(SCREEN_HEIGHT, repaintY1);
      assert(repaintY0 <= repaintY1);
      repaint_screen_lines(repaintY0, repaintY1);
      print_textsel_info();
    }
  }
}

int main(int argc, char **argv)
{
 //// query_svga_modes();
//  set_vga_text_mode(TEXT_MODE_90x60);
//  set_vga_text_mode(TEXT_MODE_40x25);
  init_screen();

  // TODO: Query current text mode to initialize defaults
  File *f = (argc > 1) ? load_file(argv[1]) : open_new_file();

  open_file_in_viewport(f, /*bringToMruFront*/1);
  assert(currentFile);
  assert(currentLine);

  init_keyboard();
  main_loop();
  deinit_keyboard();
  clear_screen(0, LTGRAY);
#if defined(__TURBOC__) && defined(__MSDOS__)
  clrscr();
#endif
  deinit_screen();
  return 0;
}
