#include "state.h"
#include "screen.h"

long allocatedMemory = 0;

File *currentFile = 0;
Line far *firstDisplayedLine = 0; // the topmost line that the view is scrolled to
unsigned long firstDisplayedLineNumber = 1;
int viewportXOffset = 0;
Line far *currentLine = 0;

// The desired X coordinate, which might not be reachable if the
// current line is too short.
int virtualX = 0;

int cursorX = 0, cursorY = 0;

int insertMode = 0;

int showWhitespace = 1;

char tabMode = ' ';

int indentSize = 2;

char tabChar = showWhitespace ? (tabMode == ' ' ? CHAR_MIDDLE_DOT : CHAR_RIGHT_ARROW) : tabMode;

unsigned char editorTextColor = LTGREEN | BGCOLOR(BLUE);

unsigned long selectionStartLineNum = 0, selectionEndLineNum = 0;
Line far *selectionStartLine = 0;
Line far *selectionEndLine = 0;
unsigned long selectionStartColumn = 0, selectionEndColumn = 0;

unsigned long current_column()
{
  return viewportXOffset + cursorX;
}

unsigned long current_line_number()
{
  return firstDisplayedLineNumber + cursorY;
}

void clear_selection()
{
  selectionStartLineNum = selectionEndLineNum = -1;
  selectionStartLine = 0;
}
