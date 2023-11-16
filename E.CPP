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
#include "c64/c64_kernal.h"
#include "c64/c64_char_rom.h"

bool scroll_viewport_horizontally_to_display_cursor();
Line far *find_last_viewport_start_line(cursor_pos *linesInView);
void scroll_to_beginning();
void scroll_to_end();
bool restrict_cursor_on_line();
void move_viewport_vertically(cursor_pos lines, bool cursor_moves_along);

#ifdef __C64__
#define COLOR_UNSAVED_CURRENT LTRED
#define COLOR_UNSAVED_BG      LTBLUE
#define COLOR_CURRENT_FILE    LTGRAY
#define COLOR_BG_FILE         LTBLUE
#define COLOR_HIGHLIGHT       0x80
#else
#define COLOR_UNSAVED_CURRENT (LTRED  | BGCOLOR(BLACK))
#define COLOR_UNSAVED_BG      (DKGRAY | BGCOLOR(BLACK))
#define COLOR_CURRENT_FILE    (LTBLUE | BGCOLOR(BLACK))
#define COLOR_BG_FILE         (DKGRAY | BGCOLOR(BLACK))
#define COLOR_HIGHLIGHT       BGCOLOR(LTGRAY)
#endif

void repaint_open_files_list(File *highlightFilename)
{
  File *f = currentFile;
  while(f->prevFile)
    f = f->prevFile;
  uint8_t x = SCREEN_WIDTH;
  while(f)
  {
    char fname[MAX_FILENAME_LEN+2]; // todo optimize away this temp copy
    assert(f->filename);
    sprintf(fname, f->hasUnsavedChanges ? "%s*" : "%s", f->filename);
    uint8_t len = (uint8_t)strlen(fname);
    if (x < len) break;
    x -= len;
    unsigned char color = (f->hasUnsavedChanges ? 
       (f == currentFile ? COLOR_UNSAVED_CURRENT : COLOR_UNSAVED_BG) :
       (f == currentFile ? COLOR_CURRENT_FILE : COLOR_BG_FILE));
    if (f == highlightFilename)
      color |= COLOR_HIGHLIGHT;

    put_string_on_screen(fname, x, 0, color);
    f = f->nextFile;
    if (f && x > 0)
      put_string_on_screen(STR_PIPE, --x, 0, LTGRAY);
  }
}

void repaint_line(int screenY, Line *line, int startX, int endX)
{
  if (line)
  {
    if (!line->colors) // XXXX TODO Want this, but syntax highlighting of multiline comments doesn't work then
    {
      startX = 0;
      syntax_highlight(line);
    }

    line_num lineNumber = firstDisplayedLineNumber + screenY;
    line_xpos selStart = (lineNumber == selectionStartLineNum) ? selectionStartColumn : (lineNumber <= selectionEndLineNum ? 0 : MAX_CHARS_PER_LINE);
    line_xpos selEnd = (lineNumber == selectionEndLineNum) ? selectionEndColumn : (lineNumber >= selectionStartLineNum ? MAX_CHARS_PER_LINE : 0);
    show_line_with_colors_with_sel(screenY * SCREEN_WIDTH, line->text, line->length,
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
      line->colors,
#endif
      selStart, selEnd, startX, endX);

    if (screenY > 0 && screenY < SCREEN_HEIGHT-1)
    {
      unsigned char existingColor = get_screen_color(screenY*SCREEN_WIDTH+SCREEN_WIDTH-1);
      if (line->length > ADD_U8_SAT(SCREEN_WIDTH, viewportXOffset))
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
  assert(firstDisplayedLine);
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
  assert(cursorY >= 0);
  assert(cursorY < SCREEN_HEIGHT);
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

void write_status_text(const char *text, unsigned char color)
{
  put_string_on_screen(text, (SCREEN_WIDTH - (cursor_pos)strlen(text))/2, SCREEN_HEIGHT-1, color);
}

void print_insert_status_text(bool print_on_clear = false)
{
  if (insertMode)
    write_status_text("insert mode", LTGRAY);
  else if (print_on_clear)
    clear_line(SCREEN_WIDTH*(SCREEN_HEIGHT-1), ' ', LTGRAY);
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

void send_current_line_to_server()
{
  send_line_to_network_server(EDIT_TYPE_REPLACE_LINE, currentFile, currentLine, current_line_number());
}

void insert_character(char ch, uint8_t times, line_xpos insert_pos)
{
  assert(currentLine);

  if (insertMode && insert_pos < currentLine->length)
  {
    assert(times == 1); // this path doesn't support multiple char inserts
    // in insert mode, the length of the text stays put, and we only replace existing
    // characters.
    currentLine->text[insert_pos] = ch;
    clear_highlight(currentLine);
    mark_current_file_modified();
    put_screen(cursorY*SCREEN_WIDTH+cursorX, ch);
    virtualX = ++cursorX;
  }
  else
  {
    // If the current cursor position is outside the end of the line,
    // we may need to insert one or more whitespace as well to create
    // a properly indented text.
    line_xpos numCharsToInsert = times;
    if (currentLine->length < insert_pos) numCharsToInsert += insert_pos - currentLine->length;

    // Check if the current line has too many characters already.
    line_xpos newLineLength = ADD_U8_SAT(currentLine->length, numCharsToInsert);
    if (newLineLength > MAX_CHARS_PER_LINE)
    {
      ERROR_BEEP();
      return;
    }

#ifdef __C64__
    // Don't do geometric vector increases on C64, it seems to be wasteful in tight memory situation
    if (!increase_capacity(currentLine, ADD_U8_SAT(newLineLength, 4)))
#else
    if (newLineLength > currentLine->capacity && !increase_capacity(currentLine, ADD_U8_SAT(newLineLength, currentLine->capacity)))
#endif
      return;

    // Shift existing characters up
    for(line_xpos i = currentLine->length + numCharsToInsert - 1; i > insert_pos; --i)
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
        assert(insert_pos - j >= 0 && insert_pos - j < currentLine->length);
        assert(numCharsToInsert-1-j >= 0 && prevLine->length && numCharsToInsert-1-j < prevLine->length);

        assert(currentLine->text);
        currentLine->text[insert_pos - j] =
          is_whitespace(prevLine->text[numCharsToInsert-1-j])
          ? prevLine->text[numCharsToInsert-1-j] : tabChar;
      }
    }

    assert(insert_pos >= 0);
    assert(insert_pos <= currentLine->length);
    assert(insert_pos < currentLine->capacity);
    assert(currentLine->text);
    currentLine->text[insert_pos] = ch;
    currentLine->length = newLineLength;
    assert(currentLine->length <= currentLine->capacity);
    clear_highlight(currentLine);
    mark_current_file_modified();
    bool full_repaint = 0;
    cursorX += times;
    if (cursorX >= SCREEN_WIDTH)
    {
      viewportXOffset += SCREEN_WIDTH-1-cursorX;
      cursorX = SCREEN_WIDTH-1;
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
  send_current_line_to_server();
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
    send_current_line_to_server();
  }
  else // join next line on this
  {
    Line far *next = currentLine->nextLine;
    if (!currentLine->nextLine)
    {
      ERROR_BEEP();
      return;
    }
    line_xpos newLineLength = ADD_U8_SAT(currentLine->length, next->length);
    if (newLineLength > MAX_CHARS_PER_LINE)
    {
      ERROR_BEEP();
      return;
    }
    increase_capacity(currentLine, newLineLength);
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
    send_line_to_network_server(EDIT_TYPE_DELETE_LINE, currentFile, 0, current_line_number()+1);
    send_current_line_to_server();
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
    if (cursorY == 0) move_viewport_vertically(-1, false);
    assert(cursorY > 0);
    currentLine = currentLine->prevLine;
    --cursorY;
    assert(get_current_line_number() == current_line_number());

    cursorX = virtualX = currentLine->length;
    delete_character();
    scroll_viewport_horizontally_to_display_cursor();
  }
  assert(cursorY >= 0);
}

// If current cursor X coordinate is too far to the right so that
// it is outside the display, we must scroll the viewport by
// adjusting viewportXOffset and repaint the screen.
// Return true if repainted screen
bool scroll_viewport_horizontally_to_display_cursor()
{
  if (currentLine->length < viewportXOffset)
  {
    viewportXOffset = currentLine->length;
    cursorX = 0;
    repaint_screen();
    return true;
  }
  if (cursorX >= SCREEN_WIDTH)
  {
    viewportXOffset += cursorX - (SCREEN_WIDTH-1);
    cursorX = SCREEN_WIDTH - 1;
    repaint_screen();
    return true;
  }
  return false;
}

// Adjust real cursor X based on the virtual cursor X of where it
// would want to be. Return true if screen was repainted.
bool restrict_cursor_on_line()
{
  cursorX = MIN(virtualX, currentLine->length - viewportXOffset);
  return scroll_viewport_horizontally_to_display_cursor();
}

void move_viewport_vertically(cursor_pos lines, bool cursor_moves_along)
{
  bool scrolled_viewport = false;

  while(lines < 0)
  {
    if (cursor_moves_along && cursorY > 0)
    {
      ++lines;
      --cursorY;
      assert(currentLine->prevLine);
      currentLine = currentLine->prevLine;
    }
    else if (firstDisplayedLine->prevLine)
    {
      ++lines;
      scrolled_viewport = true;
      firstDisplayedLine = firstDisplayedLine->prevLine;
      --firstDisplayedLineNumber;
      if (cursor_moves_along || cursorY >= SCREEN_HEIGHT-1)
        currentLine = currentLine->prevLine;
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
      if (cursor_moves_along && cursorY < SCREEN_HEIGHT-1 && currentLine->nextLine)
      {
        --lines;
        ++cursorY;
        currentLine = currentLine->nextLine;
      }
      else if (currentLine->nextLine && calculate_line_delta(firstDisplayedLine, lastPossibleViewportStartLine) > 0)
      {
        --lines;
        scrolled_viewport = true;
        firstDisplayedLine = firstDisplayedLine->nextLine;
        ++firstDisplayedLineNumber;
        if (cursor_moves_along || cursorY <= 0)
          currentLine = currentLine->nextLine;
        else
          --cursorY;
      }
      else
        break;
    }
  }
  bool repainted = restrict_cursor_on_line();
  if (!repainted) repaint_screen();
  assert(cursorY >= 0);
}

void move_cursor_vertically(cursor_pos lines)
{
  assert(currentLine);
  bool scrolled = false; // Detect if we want to play a beep sound

  if (lines < 0)
  {
    if (cursorY + lines >= 0) // No need to scroll viewport?
    {
      cursorY += lines;
      scrolled = true;
      while(lines) { ++lines; currentLine = currentLine->prevLine; assert(currentLine); }
    }
    else
    {
      if (cursorY > 0)
      {
        scrolled = true;
        lines += cursorY;
        cursorY = 0;
      }
      int8_t lines_scrolled = 0;
      while(lines && currentLine->prevLine) { firstDisplayedLine = currentLine = currentLine->prevLine; ++lines; --firstDisplayedLineNumber; ++lines_scrolled; }
      if (lines_scrolled)
      {
        scroll_screen_up(lines_scrolled);
        repaint_screen_lines(0, lines_scrolled+1); // Draw +1 line to clear the open files list at top right
        scrolled = true;
      }
    }
  }

  int8_t viewport_lines_scrolled_down = 0;
  while(lines > 0)
  {
    assert(currentLine);
    if (currentLine->nextLine)
    {
      scrolled = true;
      currentLine = currentLine->nextLine;
      if (cursorY < SCREEN_HEIGHT-1) ++cursorY;
      else
      {
        firstDisplayedLine = firstDisplayedLine->nextLine;
        ++firstDisplayedLineNumber;
        ++viewport_lines_scrolled_down;
      }
    }
    --lines;
  }
  if (viewport_lines_scrolled_down)
  {
    scroll_screen_down(viewport_lines_scrolled_down);
    repaint_screen_lines(0, 1); // Redraw first line to repaint the open files list
    repaint_screen_lines(SCREEN_HEIGHT-2, SCREEN_HEIGHT); // Redraw last line to repaint the line:column info
  }

  restrict_cursor_on_line();
  if (!scrolled) beep(170, 50);
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
    assert(viewportXOffset >= left);
    viewportXOffset -= left;
    chars += left;

    if (chars != 0)
    {
      if (currentLine->prevLine)
      {
        move_cursor_vertically(-1);
        cursorX = virtualX = currentLine->length;
        left = MAX(0, cursorX - (SCREEN_WIDTH-1));
        assert(cursorX >= left);
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

void move_cursor_to_line_start()
{
  int oldOffset = viewportXOffset;
  viewportXOffset = 0;
  cursorX = virtualX = 0;
  if (viewportXOffset != oldOffset)
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
  assert(cursorY >= 0);
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
  assert(currentLine);
  erase_selected_text();

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
  memcpy(newLine->text, currentLine->text, curLineIndent);
  memset(newLine->text + curLineIndent, tabChar, newIndent);

  // Copy leftover text from the current line to the new line
  for(line_xpos k = 0; k < newLine->length && currentCharOffset+k < currentLine->length; ++k)
    newLine->text[curLineIndent+newIndent+k] = currentLine->text[currentCharOffset+k];
  currentLine->length -= currentLine->length - currentCharOffset;
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

  send_current_line_to_server();
  send_line_to_network_server(EDIT_TYPE_INSERT_LINE, currentFile, newLine, current_line_number()+1);

  bool needRepaint = (viewportXOffset != 0);

  Line far *repaintStartLine = currentLine;
  uint8_t repaintStartY = cursorY;
  // MIN(cursorX, find_line_indent_position(currentLine));
  cursorX = virtualX = curLineIndent + newIndent;//find_line_indent_position(currentLine);
  if (cursorMovesToNewline)
  {
    if (cursorY < SCREEN_HEIGHT-1)
      ++cursorY;
    else
    {
      firstDisplayedLine = firstDisplayedLine->nextLine;
      ++firstDisplayedLineNumber;
      needRepaint = true;
    }
    currentLine = newLine;
  }
  viewportXOffset = 0;
  mark_current_file_modified();

  if (needRepaint) repaint_screen();
  else
  {
    cursor_pos endY = screen_end_y_of_last_line_on_screen(repaintStartLine, repaintStartY)+1;
    if (endY > SCREEN_HEIGHT) endY = SCREEN_HEIGHT; // XXXX REMOVE
    assert(endY <= SCREEN_HEIGHT);
    repaint_screen_lines(repaintStartY, endY);
  }
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
    if (subst == line->prevLine)
    {
      if (cursorY > 0) --cursorY;
      else move_viewport_vertically(-1, false);
    } 
    assert(cursorY >= 0);
    currentLine = subst;
  }

  send_line_to_network_server(EDIT_TYPE_DELETE_LINE, currentFile, line, get_line_number(line));

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
    if ((key == ACTION_DELETE_LEFT && inputLen > 0) || (key >= '0' && key <= '9' && inputLen < sizeof(targetLineNumberStr)-1
     && (inputLen > 0 || key != '0')))
    {
      if (key == ACTION_DELETE_LEFT)
        targetLineNumberStr[--inputLen] = 0;
      else if (inputLen < 8)
        targetLineNumberStr[inputLen++] = (char)key;

      line_num targetLineNumber = (inputLen == 0) ? origLineNumber : MIN(documentLengthLines, (line_num)atol(targetLineNumberStr));

      if (targetLineNumber < viewportPos || targetLineNumber >= viewportPos + SCREEN_HEIGHT-1)
      {
        viewportPos = MAX(1, targetLineNumber - SCREEN_HEIGHT/2);
        firstDisplayedLine = get_nth_line_of_document(viewportPos);
        firstDisplayedLineNumber = viewportPos;
      }
      currentLine = get_nth_line_of_document(targetLineNumber);
      cursorY = targetLineNumber - viewportPos;
      repaint_screen_lines(0, SCREEN_HEIGHT-1); // Don't repaint status line, that has the "GO TO LINE:" text
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
    else if (key == ACTION_ESC)
    {
      currentLine = originalCurrentLine;
      firstDisplayedLine = originalFirstDisplayedLine;
      firstDisplayedLineNumber = originalViewportPos;
      cursorY = originalCursorY;
      break;
    }
    else if (key == ACTION_ENTER)
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

void measure_and_print_available_memory()
{
  char str[64];
  sprintf(str, "%u bytes free.", malloc_free_heap_available());
  put_string_on_screen(str, (SCREEN_WIDTH-(line_xpos)strlen(str))>>1, SCREEN_HEIGHT-1, LTBLUE);
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
    if (textSelectionModeActive) sprintf(str, "text select mode");
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
  mark_text_selection_mode_inactive_and_repaint_status();
}

void highlight_scroll_to_right(bool scrolled);

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
#ifdef __C64__
    highlight_scroll_to_right(viewportXOffset != 0);
#endif
    int key = get_key_and_poll_network();

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

    switch(key)
    {
      case ACTION_CURSOR_LEFT: move_cursor_horizontally(-1); break;
      case ACTION_CURSOR_RIGHT: move_cursor_horizontally(1); break;
      case ACTION_CURSOR_UP:
        // As a navigation aid, moving up on first line should move cursor to start of line
        if (currentLine->prevLine) move_cursor_vertically(-1);
        else move_cursor_to_line_start(); break;
        break;
      case ACTION_CURSOR_DOWN:
        // As a navigation aid, moving down on last line should move cursor to end of line
        if (currentLine->nextLine) move_cursor_vertically(1);
        else move_cursor_to_line_end();
        break;
#ifdef ACTION_PAGE_ONE_THIRD_UP
#ifdef __C64__
#define SMALL_PAGE_SCROLL_LINES 8
#else
#define SMALL_PAGE_SCROLL_LINES (SCREEN_HEIGHT/3)
#endif
      case ACTION_PAGE_ONE_THIRD_UP: move_viewport_vertically(-SMALL_PAGE_SCROLL_LINES, 1); break;
#endif
#ifdef ACTION_PAGE_ONE_THIRD_DOWN
      case ACTION_PAGE_ONE_THIRD_DOWN: move_viewport_vertically(SMALL_PAGE_SCROLL_LINES, 1); break;
#endif
      case ACTION_PAGE_UP: move_viewport_vertically(-SCREEN_HEIGHT, 1); break;
      case ACTION_PAGE_DOWN: move_viewport_vertically(SCREEN_HEIGHT, 1); break;
      case ACTION_CURSOR_TO_LINE_START: toggle_cursor_home(); break;
      case ACTION_CURSOR_TO_LINE_END: move_cursor_to_line_end(); break;
      case ACTION_CURSOR_TO_PREV_WORD: move_cursor_to_prev_word(); break;
      case ACTION_CURSOR_TO_NEXT_WORD: move_cursor_to_next_word(); break;
#ifdef ACTION_VIEWPORT_UP_ONE_LINE
      case ACTION_VIEWPORT_UP_ONE_LINE: move_viewport_vertically(-1, 0); break;
#endif
#ifdef ACTION_VIEWPORT_DOWN_ONE_LINE
      case ACTION_VIEWPORT_DOWN_ONE_LINE: move_viewport_vertically(1, 0); break;
#endif
#ifdef ACTION_CURSOR_TO_FILE_START
      case ACTION_CURSOR_TO_FILE_START: scroll_to_beginning(); break;
#endif
#ifdef ACTION_CURSOR_TO_FILE_END
      case ACTION_CURSOR_TO_FILE_END: scroll_to_end(); break;
#endif
#ifdef ACTION_CURSOR_TO_TOGGLE_FILE_START_END
      case ACTION_CURSOR_TO_TOGGLE_FILE_START_END:
        if (currentLine->nextLine) scroll_to_end();
        else scroll_to_beginning(); 
        break;
#endif
      case ACTION_QUIT_PROGRAM:
#ifdef ACTION_QUIT_PROGRAM_ALT2
      case ACTION_QUIT_PROGRAM_ALT2:
#endif
#ifdef ACTION_QUIT_PROGRAM_ALT3
      case ACTION_QUIT_PROGRAM_ALT3:
#endif
        if (ask_yes_no_prompt("QUIT? "))
          running = 0;
        break;
      case ACTION_SELECT_ALL: select_all(); break;
      case ACTION_DELETE_LEFT:
        if (selectionStartLine) erase_selected_text();
        else                    backspace_character();
        break;
      case ACTION_DELETE_RIGHT:
        if (selectionStartLine) erase_selected_text();
        else                    delete_character();
        break;
#ifdef ACTION_DELETE_CURRENT_LINE
      case ACTION_DELETE_CURRENT_LINE:
#ifdef ACTION_DELETE_CURRENT_LINE_ALT2
      case ACTION_DELETE_CURRENT_LINE_ALT2:
#endif
        if (selectionStartLine) erase_selected_text();
        else                    erase_current_line();
        break;
#endif
      case ACTION_TOGGLE_INSERT_MODE:
        if (textSelectionModeActive && !insertMode)
        {
          clear_currently_selected_text();
          mark_text_selection_mode_inactive_and_repaint_status();
        }
        toggle_insert();
        break;
      case ACTION_ESC:
        clear_currently_selected_text();
        break;
      case ACTION_ENTER: insert_new_line(/*cursorMovesToNewLine=*/1); break;
#ifdef ACTION_ENTER_NEW_LINE_BEFORE_CURRENT_LINE
      case ACTION_ENTER_NEW_LINE_BEFORE_CURRENT_LINE:
        if (currentLine->prevLine)
        {
          move_cursor_vertically(-1);
          move_cursor_to_line_end();
          insert_new_line(/*cursorMovesToNewLine=*/1);
        }
        else
        {
          move_cursor_to_line_start();
          insert_new_line(/*cursorMovesToNewLine=*/0);
        }
        break;
#endif
#ifdef ACTION_ENTER_NEW_LINE_AFTER_CURRENT_LINE
      case ACTION_ENTER_NEW_LINE_AFTER_CURRENT_LINE:
        move_cursor_to_line_end();
        insert_new_line(/*cursorMovesToNewLine=*/1);
        break;
#endif
#ifdef ACTION_INSERT_INDENT
      case ACTION_INSERT_INDENT:
        insert_indent(/*line_start=*/false);
        break;
#endif
#ifdef ACTION_ADD_INDENT_TO_LINE_START
      case ACTION_ADD_INDENT_TO_LINE_START:
        insert_indent(/*line_start=*/true);
        break;
#endif
      case ACTION_UNINDENT_LINE_START: unindent(); break;
      case ACTION_ALT_TAB_MENU_NEXT_DOCUMENT:
#ifdef ACTION_ALT_TAB_MENU_NEXT_DOCUMENT_ALT2
      case ACTION_ALT_TAB_MENU_NEXT_DOCUMENT_ALT2:
#endif
        run_alt_tab_menu(); repaint_screen(); break;
#ifdef ACTION_OPEN_NEXT_DOCUMENT
      case ACTION_OPEN_NEXT_DOCUMENT: switch_to_next_file(false); break;
#endif
#ifdef ACTION_OPEN_PREV_DOCUMENT
      case ACTION_OPEN_PREV_DOCUMENT: switch_to_next_file(true); break;
#endif
      case ACTION_NEW_FILE:
      {
        File *f = open_new_file();
        if (f)
          open_file_in_viewport(f, /*bringToMruFront*/1);
      }
        break;
      case ACTION_CUT_TEXT:
#ifdef ACTION_CUT_TEXT_ALT2
      case ACTION_CUT_TEXT_ALT2:
#endif
        copy_selection(/*cutSelection=*/true);
        break;
      case ACTION_COPY_TEXT:
#ifdef ACTION_COPY_TEXT_ALT2
      case ACTION_COPY_TEXT_ALT2:
#endif
        copy_selection(/*cutSelection=*/false);
        break;
      case ACTION_PASTE_TEXT:
#ifdef ACTION_PASTE_TEXT_ALT2
      case ACTION_PASTE_TEXT_ALT2:
#endif
        paste_selection();
        break;
      case ACTION_TOGGLE_TEXT_SELECT_MODE:
        if (!textSelectionModeActive)
        {
          mark_insert_mode_inactive_and_repaint_status();
          textSelectionModeActive = true;
          print_textsel_info();
        }
        else
          copy_selection(/*cutSelection=*/false);
        break;
#ifdef ACTION_TOGGLE_TEXT_SELECT_MODE_AND_SELECT_CURRENT_LINE
      case ACTION_TOGGLE_TEXT_SELECT_MODE_AND_SELECT_CURRENT_LINE:
        if (textSelectionModeActive) copy_selection(/*cutSelection=*/false);
        else
        {
          textSelectionModeActive = true;
          move_cursor_to_line_end();
          selectionStartLineNum = selectionEndLineNum = current_line_number();
          selectionStartColumn = 0;
          selectionEndColumn = current_column();
          selectionStartLine = selectionEndLine = currentLine;
          repaint_screen_lines(selectionStartLineNum - firstDisplayedLineNumber, selectionStartLineNum + 1 - firstDisplayedLineNumber);
          print_textsel_info();
        }
        break;
#endif
      case ACTION_SAVE_FILE:
#ifdef ACTION_SAVE_FILE_AS
      case ACTION_SAVE_FILE_AS:
#endif
      {
        bool userCancelled = false;
        char device[8] = {};
        if (is_file_unnamed(currentFile) || is_shift_key_held(key))
        {
#ifdef __C64__
          userCancelled = ask_string_prompt("SAVE TO DEVICE #: ", device, 2) <= 0;
#endif
          if (!userCancelled) userCancelled = ask_name_for_file(currentFile);
          if (!userCancelled) currentFile->hasUnsavedChanges = 1;
        }
        if (!userCancelled && currentFile->hasUnsavedChanges)
        {
          save_file(currentFile, device);
          // The file name may have gotten renamed as part of the save.
          send_new_document_to_network_server(currentFile, /*clear_contents=*/false);
        }
        repaint_screen();
        send_current_line_to_server(); // Resend the current line to server to cause a rebuild and reupdate of the status
        break;
      }
      case ACTION_OPEN_GOTO_LINE_MENU: run_goto_line_menu(); break;
      case ACTION_OPEN_FILE:
        run_open_file_menu();
        repaint_screen();
        break;
      case ACTION_CLOSE_FILE:
#ifdef ACTION_CLOSE_FILE_ALT2
      case ACTION_CLOSE_FILE_ALT2:
#endif
        close_file(currentFile);
        repaint_screen();
        break;

#ifdef ACTION_ZOOM_FONT_BIGGER
      case ACTION_ZOOM_FONT_BIGGER:
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
#endif

#ifdef ACTION_ZOOM_FONT_SMALLER
      case ACTION_ZOOM_FONT_SMALLER:
        if (currentVideoMode < LAST_VIDEO_MODE)
        {
          set_vga_text_mode(++currentVideoMode);
          repaint_screen();
        }
        break;
#endif

#ifdef ACTION_OPEN_COMMAND_LINE
      case ACTION_OPEN_COMMAND_LINE:
        clear_screen(' ', LTGRAY);
        clrscr();
        system("command.com");
        repaint_screen();
        break;
#endif
#if defined(ACTION_SHOW_AVAILABLE_MEMORY)// && !defined(NDEBUG)
      case ACTION_SHOW_AVAILABLE_MEMORY:
        measure_and_print_available_memory();
        break;
#endif
#ifdef ACTION_REPAINT_SCREEN
      case ACTION_REPAINT_SCREEN:
        repaint_screen();
        break;
#endif
#ifdef ACTION_SAVE_COMPILED_PRG_TO_DISK
      case ACTION_SAVE_COMPILED_PRG_TO_DISK:
#ifdef ACTION_SAVE_COMPILED_PRG_TO_DISK_ALT2
      case ACTION_SAVE_COMPILED_PRG_TO_DISK_ALT2:
#endif
        {
        send_ask_compiled_document_msg(currentFile->document_id, /*for_loading_to_memory=*/false);
        char filename[20];
        wait_to_receive_file(filename);
        repaint_screen();
        char text[40];
        sprintf(text, "program %s saved to disk", filename);
        write_status_text(text, LTGREEN);
        }
        break;
#endif

#ifdef ACTION_RUN_COMPILED_PRG
      case ACTION_RUN_COMPILED_PRG:
#ifdef ACTION_RUN_COMPILED_PRG_ALT2
      case ACTION_RUN_COMPILED_PRG_ALT2:
#endif
        send_ask_compiled_document_msg(currentFile->document_id, /*for_loading_to_memory=*/true);
        receive_and_run_prg(/*run_prg=*/true);
        repaint_screen();
#endif
        break;

      default:
        if (is_printable_key(key))
        {
          // Autocomplete: when typing a '}' character, automatically unindent the curly brace
          // to one indent level lower.
          if (key == RIGHT_CURLY_BRACE && all_chars_left_of_cursor_are_whitespace())
          {
            // .. but only unindent if cursor is at least the x level of previous indentation.
            if (currentLine->prevLine && find_line_indent_position(currentLine->prevLine) <= current_column())
              unindent();
          }
          if (showWhitespace && key == ' ') key = CHAR_MIDDLE_DOT;
          erase_selected_text();
          insert_character(key, 1, current_column());
        }
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

#ifdef __C64__

extern "C" size_t __heap_start;
extern "C" char __restore_overlay_start, __restore_overlay_dst, __restore_overlay_dst_end;
extern "C" void malloc_add_free_block(void *ptr, size_t size);

int main()
#else
int main(int argc, char **argv)
#endif
{
#ifdef __C64__
  deploy_restore_overlay_memory(); // This must be done before initializing the heap
  setup_cpp_programming_font();
  malloc_add_free_block((void*)0x0400, 0x0400);
#endif
 //// query_svga_modes();
//  set_vga_text_mode(TEXT_MODE_90x60);
//  set_vga_text_mode(TEXT_MODE_40x25);
  init_network();
  init_screen();

  // TODO: Query current text mode to initialize defaults
#ifdef __C64__
  File *f = open_new_file();
#else
  File *f = (argc > 1) ? load_file(argv[1]) : open_new_file();
#endif

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
#ifdef __C64__
  _RESET();
#endif
}
