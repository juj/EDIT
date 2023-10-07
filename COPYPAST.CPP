#include "copypast.h"
#include "util.h"
#include "memory.h"
#include "state.h"
#include "line.h"
#include "highlite.h"
#include "file.h"
#include "screen.h"
#include "audio.h" // TODO REMOVE THIS
#include <assert.h>
#include <string.h>

char huge *copyBuffer = 0;
long copyBufferLength = 0;
long copyBufferCapacity = 0;

void repaint_screen_lines(int yStart, int yEnd); // TODO proper declaration location

void calculate_num_lines_and_chars_selected(long *lines, long *chars)
{
  assert(lines);
  assert(chars);

  assert(selectionEndLine);
  assert(selectionStartColumn <= selectionStartLine->length);
  long numLinesSelected = 0;
  long numCharsSelected;
  if (selectionStartLineNum != selectionEndLineNum)
  {
    numCharsSelected = selectionStartLine->length - selectionStartColumn;
    if (numCharsSelected > 0) ++numLinesSelected;
    for(Line far *l = selectionStartLine->nextLine; l != selectionEndLine; l = l->nextLine)
    {
      assert(l);
      numCharsSelected += l->length;
      ++numLinesSelected;
    }
    if (selectionEndColumn > 0) ++numLinesSelected;
    numCharsSelected += selectionEndColumn;
  }
  else
  {
    numLinesSelected = 1;
    numCharsSelected = selectionEndColumn - selectionStartColumn;
  }
  assert(selectionEndColumn <= selectionEndLine->length);
  *lines = numLinesSelected;
  *chars = numCharsSelected;
}

static void copy_buffer_reserve(long newCap)
{
  if (copyBufferCapacity < newCap)
  {
    if (copyBuffer) Farfree(copyBuffer, copyBufferCapacity);
    copyBuffer = (char huge *)Farmalloc(newCap);
    copyBufferCapacity = copyBuffer ? newCap : 0;
  }
}

void copy_selection()
{
  long numLinesSelected;
  long numCharsSelected;
  calculate_num_lines_and_chars_selected(&numLinesSelected, &numCharsSelected);
  if (numCharsSelected == 0)
  {
    copyBufferLength = 0;
    return;
  }
  copy_buffer_reserve(numCharsSelected + numLinesSelected + 1);
  if (!copyBuffer)
    return;

  // Copy text to copy buffer
  long offset = selectionStartColumn;
  long dstOfs = 0;
  Line far *l = selectionStartLine;
  for(long lineNum = selectionStartLineNum; lineNum <= selectionEndLineNum;
    ++lineNum, l = l->nextLine)
  {
    if (!l)
      break;

    long end = (lineNum == selectionEndLineNum) ? selectionEndColumn : l->length;
    assert(offset <= end);
    long len = end - offset;
//    printf("dstOfs: %lu, len: %lu, copybufferCapacity: %lu, lineNum: %lu, selectionEndLineNum: %lu\n",
 //     dstOfs, len, copyBufferCapacity, lineNum, selectionEndLineNum);
    assert(dstOfs + len <= copyBufferCapacity);
    _fmemcpy(&copyBuffer[dstOfs], &l->text[offset], len);
    dstOfs += len;
    if (lineNum != selectionEndLineNum)
    {
      assert(dstOfs+1 <= copyBufferCapacity);
      copyBuffer[dstOfs++] = '\n';
    }
    offset = 0;
  }
  assert(dstOfs <= copyBufferCapacity);
  copyBufferLength = dstOfs;
}

void repaint_screen(); // TODO REMOVE
void erase_selected_text()
{
  assert(selectionStartLine);
  assert(selectionEndLine);
  if (selectionStartLineNum == selectionEndLineNum) // Only a single line of selected text?
  {
    unsigned long newLineLength = selectionStartLine->length - (selectionEndColumn - selectionStartColumn);
    for(size_t i = 0; i < selectionStartLine->length - selectionEndColumn; ++i)
      selectionStartLine->text[selectionStartColumn+i] = selectionStartLine->text[selectionEndColumn+i];
    selectionStartLine->length = newLineLength;
    cursorX = virtualX = MIN(cursorX, selectionStartColumn);
    clear_selection();
    repaint_screen_lines(cursorY, cursorY+1);
    return;
  }

  // Join end line to start line
  unsigned long endLenRemain = selectionEndLine->length - selectionEndColumn;
  unsigned long newLen = selectionStartColumn + endLenRemain;
  if (!increase_capacity(selectionStartLine, newLen))
    return;
  selectionStartLine->length = newLen;
  for(size_t i = 0; i < endLenRemain; ++i)
    selectionStartLine->text[selectionStartColumn+i] = selectionEndLine->text[selectionEndColumn+i];
/*
  // Cut end line
  unsigned long newEndLineLength = selectionEndLine->length - selectionEndColumn;
  for(size_t i = 0; i < selectionEndLine->length - selectionEndColumn; ++i)
    selectionEndLine->text[i] = selectionEndLine->text[selectionEndColumn+i];
  selectionEndLine->length = newEndLineLength;
  */
  // Free middle lines
  Line far *l = selectionStartLine->nextLine;
  assert(l);
  Line far *endLine = selectionEndLine->nextLine;
  for(long j = selectionStartLineNum+1; j <= selectionEndLineNum && l; ++j)
  {
    Line far *next = l->nextLine;
    if (l == firstDisplayedLine)
    {
      firstDisplayedLine = selectionStartLine;
      firstDisplayedLineNumber = selectionStartLineNum;
      cursorY = 0;
    }
    delete_line(l);
    l = next;
  }
  selectionStartLine->nextLine = endLine;
  if (endLine) endLine->prevLine = selectionStartLine;
  else currentFile->lastLine = selectionStartLine;
  cursorX = virtualX = selectionStartColumn;
  currentLine = selectionStartLine;
  cursorY = get_line_number(currentLine) - firstDisplayedLineNumber;
  clear_selection();
  repaint_screen(); // TODO: optimize smarter repaint
}

void cut_selection()
{
  copy_selection();
  erase_selected_text();
}

void repaint_screen_lines(int yStart, int yEnd);
void repaint_screen();
void insert_new_line(int cursorMovesToNewline);
void restrict_cursor_on_line();

void paste_selection()
{
  if (copyBufferLength == 0) return;

  // Find the first line of content, and paste it on the
  // current cursor line
  long i = 0;
  for(; i < copyBufferLength; ++i)
    if (copyBuffer[i] == '\n')
      break;

  if (!increase_capacity(currentLine, currentLine->length + i))
    return;

  long pastePosition = current_column();
  if (i < copyBufferLength)// && pastePosition < currentLine->length) // if true, we will be pasting multiple lines
  {
    insert_new_line(0); // TODO: Disable indent generation here?
    cursorX = virtualX = find_line_indent_position(currentLine);
    viewportXOffset = 0;
  }
  else
  {
    cursorX += i; // Advance cursor on a single line paste
    virtualX = cursorX;
  }

  if (i > 0)
  {
    for(long k = currentLine->length + i - 1; k >= pastePosition + i; --k)
      currentLine->text[k] = currentLine->text[k - i];
    for(size_t l = 0; l < i; ++l)
      currentLine->text[pastePosition+l] = copyBuffer[l];
    currentLine->length += i;
  }
  clear_highlight(currentLine);

  if (i >= copyBufferLength)
  {
    repaint_screen_lines(cursorY, cursorY+1);
    return;
  }
  ++i; // Skip first newline

  Line far *startLine = currentLine;
  Line far *endLine = currentLine->nextLine;
  // Generate new lines after the current line
  unsigned long numPastedLines = 1;
  while (i < copyBufferLength) // multi-line pasting:
  {
    long j;
    for(j = i; j < copyBufferLength; ++j)
      if (copyBuffer[j] == '\n')
    break;

    if (j == copyBufferLength)// && j-i > 0) // last line?
    {
      // prepend the last block at the beginning of the last line
      long pasteAmount = j-i;
      if (!increase_capacity(endLine, endLine->length + pasteAmount))
        return;
      for(long k = endLine->length + pasteAmount-1; k >= pasteAmount; --k)
        endLine->text[k] = endLine->text[k - pasteAmount];
      for(size_t l = 0; l < pasteAmount; ++l)
        endLine->text[l] = copyBuffer[i+l];
      endLine->length += pasteAmount;
      clear_highlight(endLine);
      cursorX = virtualX = pasteAmount;
      viewportXOffset = 0; // TODO handle scrolling
      break;
    }

    Line far *newLine = (Line far *)Farmalloc(sizeof(Line));
    if (!newLine)
      return;
    newLine->length = newLine->capacity = j - i;
    newLine->text = newLine->length > 0 ? (char huge *)Farmalloc(newLine->length) : 0;
#ifdef ENABLE_SYNTAX_HIGHLIGHTING
    newLine->colors = 0;
#endif
    if (!newLine->text && newLine->length > 0)
    {
      delete_line(newLine);
      return;
    }
    for(size_t k = 0; k < j-i; ++k)
      newLine->text[k] = copyBuffer[i+k];
    newLine->ownsPtr = 1;
    newLine->prevLine = startLine;
    startLine->nextLine = newLine;
    startLine = newLine;
    ++numPastedLines;
    i = j+1;
    assert(i <= copyBufferLength+1);
    cursorX = virtualX = 0;
  }
  startLine->nextLine = endLine;
  if (endLine) endLine->prevLine = startLine;
  else currentFile->lastLine = startLine;

  repaint_screen_lines(cursorY, MIN(SCREEN_HEIGHT, cursorY+numPastedLines));
  currentLine = endLine;
  cursorY += numPastedLines;
  viewportXOffset = 0; // TODO proper horiz scrolling

  repaint_screen(); // TODO: optimize repaint
}
