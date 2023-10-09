#include "state.h"
#include "screen.h"

allocated_mem_size_t allocatedMemory = 0;

File *currentFile = 0;
Line far *firstDisplayedLine = 0; // the topmost line that the view is scrolled to
line_num firstDisplayedLineNumber = 1;
Line far *currentLine = 0;

line_xpos viewportXOffset = 0;

// The desired X coordinate, which might not be reachable if the
// current line is too short.
line_xpos virtualX = 0;

line_xpos cursorX = 0;
cursor_pos cursorY = 0;

bool insertMode = 0;
bool textSelectionModeActive = 0;

bool showWhitespace = 0;

// If true, tab mode is set to spaces, if false, to tabs.
bool tabModeIsSpaces = 1;

line_xpos indentSize = 2;

char tabChar = showWhitespace ? (tabModeIsSpaces ? CHAR_MIDDLE_DOT : CHAR_RIGHT_ARROW) : (tabModeIsSpaces ? ' ' : (char)9);

unsigned char editorTextColor = LTGREEN | BGCOLOR(BLUE);

long selectionStartLineNum = 0, selectionEndLineNum = 0;
Line far *selectionStartLine = 0;
Line far *selectionEndLine = 0;
long selectionStartColumn = 0, selectionEndColumn = 0;

long current_column()
{
  return viewportXOffset + cursorX;
}

long current_line_number()
{
  return firstDisplayedLineNumber + cursorY;
}

void clear_selection()
{
  selectionStartLineNum = selectionEndLineNum = -1;
  selectionStartLine = 0;
}
