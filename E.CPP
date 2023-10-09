#include <stdio.h>
#include <stdlib.h>
#include "iassert.h"
#if defined(_MSC_VER) || defined(__TURBOC__)
#include <conio.h>
#include <dos.h>
#endif
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
#include "savefile.h"
#include "viewport.h"
#include "memory.h"
#include "copypast.h"
#include "highlite.h"
#include "cursor.h"
#include "smindent.h"
#include "network.h"

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
       (f == currentFile ? LTBLUE : LTGRAY));
    unsigned char bgColor = (f == highlightFilename) ? BGCOLOR(LTGRAY) : BGCOLOR(BLACK);

    put_string_on_screen(fname, x, 0, foreColor | bgColor);
    f = f->nextFile;
    if (f)
    {
      put_string_on_screen("|", --x, 0, LTGRAY);
    }
  }
}

void repaint_line(int screenY, Line *line, int startX, int endX)
{
  if (line)
  {
    //if (!line->colors) // XXXX TODO Want this, but syntax highlighting of multiline comments doesn't work then
    {
      startX = 0;
      syntax_highlight(line);
    }

    line_num lineNumber = firstDisplayedLineNumber + screenY;
    line_xpos selStart = (lineNumber == selectionStartLineNum) ? selectionStartColumn : (lineNumber <= selectionEndLineNum ? 0 : MAX_CHARS_PER_LINE);
    line_xpos selEnd = (lineNumber == selectionEndLineNum) ? selectionEndColumn : (lineNumber >= selectionStartLineNum ? MAX_CHARS_PER_LINE : 0);
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
    if (line->colors) show_line_with_colors_with_sel(screenY * SCREEN_WIDTH, line->text, line->length, line->colors, selStart, selEnd, startX, endX);
    else
#endif
      show_line_with_sel(screenY * SCREEN_WIDTH, line->text, line->length, selStart, selEnd, startX, endX);

    if (screenY > 0 && screenY < SCREEN_HEIGHT-1)
    {
      unsigned char existingColor = get_screen_color(screenY*SCREEN_WIDTH+SCREEN_WIDTH-1);
      if (line->length > SCREEN_WIDTH + viewportXOffset)
        put_screen_color(screenY*SCREEN_WIDTH+SCREEN_WIDTH-1, existingColor | BGCOLOR(BROWN));
      else
        put_screen_color(screenY*SCREEN_WIDTH+SCREEN_WIDTH-1, existingColor & ~BGCOLOR(BROWN));
    }
  }
  else
    clear_line(screenY * SCREEN_WIDTH, 0);

  if (viewportXOffset != 0)
    put_screen_color(screenY*SCREEN_WIDTH, get_screen_color(screenY*SCREEN_WIDTH) | BGCOLOR(BROWN));
}

void repaint_screen_lines(cursor_pos yStart, cursor_pos yEnd)
{
  assert(yStart >= 0);
  assert(yStart <= SCREEN_HEIGHT);
  assert(yStart <= yEnd);
  assert(yEnd >= 0);
  assert(yEnd <= SCREEN_HEIGHT);
  Line far *curLine = get_nth_line_on_screen(yStart);
  cursor_pos screenY = yStart;
  line_num lineNumber = firstDisplayedLineNumber + yStart;

  for(; screenY < yEnd; ++screenY)
  {
    repaint_line(screenY, curLine, 0, SCREEN_WIDTH);
    if (curLine) curLine = curLine->nextLine;
    ++lineNumber;
  }
  if (yStart == 0)
    repaint_open_files_list(0);
}

void print_line_column_info()
{
  assert(get_current_line_number() == current_line_number());
  //  line_num curLineNumber = get_current_line_number();
  char str[32];
  /*
  sprintf(str, " %lub near/%lub far mem used    %ld:%d",
  allocatedNearMemory, allocatedFarMemory,
  curLineNumber, current_column()+1);
  */

  sprintf(str, " " LINE_NUM_PRINTF_MOD ":" LINE_XPOS_PRINTF_MOD, current_line_number(), (int)(current_column()+1));

  put_string_on_screen(str, SCREEN_WIDTH - (cursor_pos)strlen(str), SCREEN_HEIGHT-1, LTGRAY);
}

void repaint_screen()
{
  repaint_screen_lines(0, SCREEN_HEIGHT);
  print_line_column_info();
  set_cursor_pos(cursorX, cursorY);
}

void verify_screen_is_painted_correctly()
{
#ifdef DEBUG_INCREMENTAL_REPAINTING
  print_line_column_info(); // this might be out of date, since we print int lazily only later to save code size
  save_screen();
  repaint_screen();
  assert(saved_screen_matches_current_screen());
  set_cursor_pos(cursorX, cursorY);
#endif
}

void repaint_current_line_from_offset(cursor_pos xOffset, cursor_pos xEnd)
{
  // TODO: actually paint from current offset onwards only
  repaint_line(cursorY, currentLine, xOffset, xEnd);
  if (cursorY == 0)
    repaint_open_files_list(0);
  //repaint_screen_lines(cursorY, cursorY+1);
}

void insert_character(char ch)
{
  assert(currentLine);
  line_xpos currentCharOffset = current_column();

  if (insertMode && currentCharOffset < currentLine->length)
  {
    currentLine->text[currentCharOffset] = ch;
    clear_highlight(currentLine);
    mark_current_file_modified();
    put_screen(cursorY*SCREEN_WIDTH+cursorX, ch);
    ++cursorX;
    virtualX = cursorX;
  }
  else
  {
    // If the current cursor position is outside the end of the line,
    // we may need to insert one or more whitespace as well to create
    // a properly indented text.
    line_xpos numCharsToInsert = 1 + MAX(0, currentCharOffset - currentLine->length);

    if (currentLine->length + numCharsToInsert > currentLine->capacity)
    {
      if (!increase_capacity(currentLine, currentLine->capacity*2+numCharsToInsert))
        return;
    }

    // Shift existing characters up
    for(line_xpos i = currentLine->length + numCharsToInsert - 1; i > currentCharOffset; --i)
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
      for(line_xpos j = 1; j < numCharsToInsert; ++j)
      {
        assert(currentCharOffset - j >= 0 && currentCharOffset - j < currentLine->length);
        assert(numCharsToInsert-1-j >= 0 && prevLine->length && numCharsToInsert-1-j < prevLine->length);

        assert(currentLine->text);
        currentLine->text[currentCharOffset - j] =
          is_whitespace(prevLine->text[numCharsToInsert-1-j])
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
    mark_current_file_modified();
    bool full_repaint = 0;
    if (++cursorX >= SCREEN_WIDTH)
    {
      --cursorX;
      ++viewportXOffset;
      full_repaint = true;
    }
    virtualX = cursorX;

    if (full_repaint) repaint_screen();
    else
    {
      repaint_current_line_from_offset(cursorX-1, currentLine->length);
      verify_screen_is_painted_correctly();
    }
  }
}

void delete_character()
{
  assert(currentLine);
  line_xpos currentCharOffset = current_column();
  if (currentCharOffset < currentLine->length)
  {
    while(currentCharOffset+1 < currentLine->length)
    {
      currentLine->text[currentCharOffset] = currentLine->text[currentCharOffset+1];
      ++currentCharOffset;
    }
    --currentLine->length;
    clear_highlight(currentLine);
    repaint_current_line_from_offset(MAX(cursorX-1, 0), currentLine->length+1);
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
    for(line_xpos i = 0; i < next->length; ++i)
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
    clear_highlight(currentLine);

    cursor_pos endY = screen_end_y_of_last_line_on_screen(currentLine, cursorY)+2;
    if (endY > SCREEN_HEIGHT) endY = SCREEN_HEIGHT;
    repaint_screen_lines(cursorY, endY);
  }
  mark_current_file_modified();
  // TODO: only repaint the current line starting from currentCharOffset
  //if (full_repaint) repaint_screen();
  /*else*/// repaint_current_line_from_offset(cursorX-1);
  verify_screen_is_painted_correctly();
}

void backspace_character()
{
  int currentCharOffset = current_column();
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

Line far *find_last_viewport_start_line(cursor_pos *linesInView);
void scroll_to_beginning();
void scroll_to_end();
void restrict_cursor_on_line();

void move_viewport_vertically(cursor_pos lines, bool cursor_moves_along)
{
  bool scrolled = 0;

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

  if (scrolled) repaint_screen();
}

void move_cursor_vertically(cursor_pos lines)
{
  bool makeSound = 0;

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

void move_cursor_horizontally(cursor_pos chars)
{
  cursor_pos oldViewportXOffset = viewportXOffset;

  if (chars < 0)
  {
    cursor_pos left = MIN(cursorX, -chars);
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
    cursor_pos right = MIN(currentLine->length - viewportXOffset - cursorX, chars);
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
  cursor_pos indentPos = find_line_indent_position(currentLine);
  cursorX = virtualX = (cursorX != indentPos) ? indentPos : 0;

  cursor_pos oldViewportXOffset = viewportXOffset;
  viewportXOffset = MAX(0, cursorX - SCREEN_WIDTH);
  if (oldViewportXOffset != viewportXOffset)
    repaint_screen();
}

Line far *find_last_viewport_start_line(cursor_pos *linesInView)
{
  Line far *lastLine = currentFile->lastLine;
  cursor_pos y = 0;
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
  line_xpos currentCharOffset = current_column();
  Line far *line = currentLine;

  if (currentCharOffset == 0) // If cursor is at beginning of a line, move it to the end of the previous line
  {
    if (!line->prevLine)
    {
      ERROR_BEEP();
      return;
    }
    currentLine = currentLine->prevLine;
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
    while(currentCharOffset > 0)
    {
      --currentCharOffset;
      if (is_alphanum(line->text[currentCharOffset])
        && (currentCharOffset == 0 || !is_alphanum(line->text[currentCharOffset-1])))
        break;
    }
    line_xpos oldXOffset = viewportXOffset;
    viewportXOffset = MIN(viewportXOffset, currentCharOffset);
    cursorX = virtualX = currentCharOffset - viewportXOffset;
    if (oldXOffset != viewportXOffset)
      repaint_screen();
  }
}

void move_cursor_to_next_word()
{
  line_xpos currentCharOffset = current_column()+1;
  Line far *line = currentLine;

  if (currentCharOffset >= line->length) // If cursor is at end of a line, move it to the start of the next line
  {
    if (!line->nextLine)
    {
      ERROR_BEEP();
      return;
    }
    currentLine = currentLine->nextLine;
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
    for(; currentCharOffset < line->length; ++currentCharOffset)
      if (is_alphanum(line->text[currentCharOffset])
        && !is_alphanum(line->text[currentCharOffset-1]))
      break;

    line_xpos oldXOffset = viewportXOffset;
    viewportXOffset = MIN(viewportXOffset, currentCharOffset);
    cursorX = virtualX = currentCharOffset - viewportXOffset;
    if (oldXOffset != viewportXOffset)
      repaint_screen();
  }
}

// Inserts a new line after the current one, e.g. when enter is pressed (maybe in the middle of a line)
void insert_new_line(bool cursorMovesToNewline)
{
  line_xpos currentCharOffset = current_column();

  // TODO: Also guess indent here
  line_xpos newIndent = (last_nonws_char_on_line(currentLine, currentCharOffset) == LEFT_CURLY_BRACE) ? indentSize : 0;

  // We copy indent from current line to the new line up to
  // the character position
  line_xpos curLineIndent = MIN(currentCharOffset, find_line_indent_position(currentLine));

  Line far *newLine = (Line far *)Farmalloc(sizeof(Line));
  if (!newLine) return; // OOM!

  // move all chars from current line cursor position onwards to new line

  // TODO: With a smart shared pointer handling of the char data,
  // could split here with a no-alloc scheme.

  newLine->length = newLine->capacity = curLineIndent + newIndent + currentLine->length - currentCharOffset;
  newLine->text = newLine->length > 0 ? (char huge *)Farmalloc(newLine->length) : 0;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
  newLine->colors = 0;
  newLine->colorFlags = 0;
#endif
  if (!newLine->text && newLine->length > 0)
  {
    delete_line(newLine);
    return;
  }
  // Copy indent
  for(line_xpos i = 0; i < curLineIndent; ++i)
    newLine->text[i] = currentLine->text[i];
  for(line_xpos j = 0; j < newIndent; ++j)
    newLine->text[curLineIndent+j] = tabChar;

  // Copy leftover text from the current line to the new line
  for(line_xpos k = 0; k < newLine->length && currentCharOffset+k < currentLine->length; ++k)
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

  mark_current_file_modified();

  Line *prevLine = currentLine->prevLine;
  assert(prevLine);
  cursor_pos endY = screen_end_y_of_last_line_on_screen(prevLine, cursorY-1)+1;
  if (endY > SCREEN_HEIGHT) endY = SCREEN_HEIGHT;
  repaint_screen_lines(cursorY-1, endY);
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
  line_num origLineNumber = get_current_line_number();
  char targetLineNumberStr[32] = {0};

  if (viewportXOffset > 0)
  {
    viewportXOffset = 0;
    repaint_screen();
  }

  put_string_on_screen("GO TO LINE: ", 0, SCREEN_HEIGHT-1, LTGRAY);
  char origLineNumberStr[32];
  sprintf(origLineNumberStr, LINE_NUM_PRINTF_MOD " ", origLineNumber);
  put_string_on_screen(origLineNumberStr, 12, SCREEN_HEIGHT-1, DKGRAY);
  set_cursor_pos(12, SCREEN_HEIGHT-1);
  const char *empty = " ";

//  Line far *originalFirstDisplayedLine = firstDisplayedLine;
  Line far *originalCurrentLine = currentLine;
  Line far *originalFirstDisplayedLine = firstDisplayedLine;
  cursor_pos originalCursorY = cursorY;
  line_num originalViewportPos = get_line_number(firstDisplayedLine);
  line_num viewportPos = originalViewportPos;
  highlight_line(origLineNumber - originalViewportPos, BGCOLOR(LTGRAY));
  int8_t inputLen = 0;
  line_num documentLengthLines = get_line_number(currentFile->lastLine);
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

      line_num targetLineNumber = (inputLen == 0) ? origLineNumber : MIN(documentLengthLines, atol(targetLineNumberStr));

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
#ifndef __C64__ // TODO
    else if (key == KEY_ESC)
    {
      currentLine = originalCurrentLine;
      firstDisplayedLine = originalFirstDisplayedLine;
      firstDisplayedLineNumber = originalViewportPos;
      cursorY = originalCursorY;
      break;
    }
#endif
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

void print_textsel_info()
{
  // TODO: Can this be optimized to occur less frequently?
  repaint_screen_lines(SCREEN_HEIGHT-1, SCREEN_HEIGHT);
  if (!selectionStartLine && !textSelectionModeActive)
    return;

  assert(selectionStartLineNum <= selectionEndLineNum);
  line_num numLinesSelected;
  file_idx numCharsSelected;
  calculate_num_lines_and_chars_selected(&numLinesSelected, &numCharsSelected);

  char str[64];
  if (numCharsSelected == 0)
  {
    if (textSelectionModeActive) sprintf(str, "SELECT");
    else str[0] = 0;
  }
  else if (numLinesSelected == 1)
    sprintf(str, FILE_IDX_PRINTF_MOD " chars selected.", numCharsSelected);
  else
    sprintf(str, LINE_NUM_PRINTF_MOD " lines, " FILE_IDX_PRINTF_MOD " chars selected.", numLinesSelected, numCharsSelected);
  put_string_on_screen(str, (SCREEN_WIDTH-(line_xpos)strlen(str))>>1, SCREEN_HEIGHT-1, LTBLUE);
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

void clear_currently_selected_text()
{
  if (selectionStartLine)
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

void main_loop()
{
  clear_screen(' ', editorTextColor);
  repaint_screen();
  bool running = 1;

  line_xpos oldCursorPos;
  line_num oldLineNum;
  Line far *oldLine;

  while(running)
  {
    print_line_column_info();
    set_cursor_pos(cursorX, cursorY);

    assert(firstDisplayedLine);
    assert(get_line_number(firstDisplayedLine) == firstDisplayedLineNumber);
    assert(currentLine);
    assert(get_line_number(currentLine) == current_line_number());
//    repaint_screen();
    int key = get_key();

    // Doing text selection?
    bool textSelection = 0;
    if (is_navigation_action_key(key))
    {
#ifdef __C64__
      // C64 does not support shift-keys for movement.
      textSelection = textSelectionModeActive;
#else
      textSelection = textSelectionModeActive || is_shift_key_held(key);
#endif
      if (textSelection)
      {
        oldCursorPos = current_column();
        oldLineNum = current_line_number();
        oldLine = currentLine;
        // start new text selection
        if (!selectionStartLine)
        {
          selectionStartLine = selectionEndLine = oldLine;
          selectionStartLineNum = selectionEndLineNum = oldLineNum;
          selectionStartColumn = selectionEndColumn = oldCursorPos;
        }
      }
      // If text selection mode is not active, and cursor is moved,
      // clear any previous text selection
      else clear_currently_selected_text();
    }

    switch(key & ~KEY_SHIFT_MODIFIER)
    {
      case KEY_LEFT: move_cursor_horizontally(-1); break;
      case KEY_RIGHT: move_cursor_horizontally(1); break;
      case KEY_UP: move_cursor_vertically(-1); break;
      case KEY_DOWN: move_cursor_vertically(1); break;
#ifndef __C64__ // TODO
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
#endif
      case KEY_CTRL_A: select_all(); break;
      case KEY_BACKSPACE:
        if (selectionStartLine)
          erase_selected_text();
        else
          backspace_character();
        break;
      case KEY_DELETE:
        if (key == KEY_SHIFT_DELETE)
          cut_selection();
        else if (selectionStartLine)
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
      case KEY_INSERT:
        if (key == KEY_SHIFT_INSERT)
          paste_selection(); // TODO: this does not work, Shift-Ins generates a zero instead.
        else
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
      case KEY_F1:
        textSelectionModeActive = !textSelectionModeActive;
        print_textsel_info();
        break;
      case KEY_CTRL_S:
      case KEY_F2:
        if (currentFile->hasUnsavedChanges)
        {
          if (is_file_unnamed(currentFile) || is_shift_key_held(key)) ask_name_for_file(currentFile);
          save_file(currentFile);
          save_file_to_network(currentFile);
          repaint_screen();
        }
        break;
      case KEY_CTRL_G: run_goto_line_menu(); break;
      case KEY_CTRL_O:
      case KEY_F3:
        run_open_file_menu();
        repaint_screen();
        break;
      case KEY_CTRL_W:
#ifndef __C64__ // TODO
      case KEY_CTRL_F4:
#endif
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
        clear_screen(' ', LTGRAY);
        clrscr();
        system("command.com");
        repaint_screen();
        break;
#endif

      default:
        if (key == RIGHT_CURLY_BRACE && all_chars_left_of_cursor_are_whitespace()) unindent();
        if (showWhitespace && key == ' ') key = CHAR_MIDDLE_DOT;
        insert_character(key);
        break;
    }
    if (textSelection)
    {
      line_xpos cursorPos = current_column();
      line_num lineNum = current_line_number();
      Line far *line = currentLine;
      int8_t oldCmpStart = cursor_pos_cmp(oldLineNum, oldCursorPos, selectionStartLineNum, selectionStartColumn);
      if (!oldCmpStart)
      {
        int8_t newCmpEnd = cursor_pos_cmp(lineNum, cursorPos, selectionEndLineNum, selectionEndColumn);
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
        int8_t startCmpNew = cursor_pos_cmp(selectionStartLineNum, selectionStartColumn, lineNum, cursorPos);
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
      cursor_pos repaintY0 = (cursor_pos)(MIN(oldLineNum, lineNum) - firstDisplayedLineNumber);
      repaintY0 = MAX(0, repaintY0);
      cursor_pos repaintY1 = (cursor_pos)(MAX(oldLineNum, lineNum) + 1 - firstDisplayedLineNumber);
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
  init_network();
  init_screen();

  // TODO: Query current text mode to initialize defaults
  File *f = (argc > 1) ? load_file(argv[1]) : open_new_file();

  open_file_in_viewport(f, /*bringToMruFront*/1);
  assert(currentFile);
  assert(currentLine);

  init_keyboard();
  set_cursor_style(CURSOR_NORMAL);
  main_loop();
  deinit_keyboard();
  clear_screen(' ', LTGRAY);
#if defined(__TURBOC__) && defined(__MSDOS__)
  clrscr();
#endif
  deinit_screen();
  deinit_network();
  return 0;
}
