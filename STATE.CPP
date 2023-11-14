#include "state.h"
#include "screen.h"
#include <assert.h>

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

bool insertMode = false;
bool textSelectionModeActive = false;

bool showWhitespace = false;

// If true, tab mode is set to spaces, if false, to tabs.
bool tabModeIsSpaces = true;

line_xpos indentSize = 2;

char tabChar = showWhitespace ? (tabModeIsSpaces ? CHAR_MIDDLE_DOT : CHAR_RIGHT_ARROW) : (tabModeIsSpaces ? ' ' : (char)9);

unsigned char editorTextColor = LTGREEN | BGCOLOR(BLUE);

line_num selectionStartLineNum = 0, selectionEndLineNum = 0;
Line far *selectionStartLine = 0;
Line far *selectionEndLine = 0;
line_xpos selectionStartColumn = 0, selectionEndColumn = 0;

line_xpos current_column()
{
  return viewportXOffset + cursorX;
}

line_num current_line_number()
{
  assert(cursorY >= 0);
  assert(cursorY < SCREEN_HEIGHT);
  return firstDisplayedLineNumber + cursorY;
}

void repaint_screen_lines(cursor_pos yStart, cursor_pos yEnd); // TODO proper declaration location

void repaint_status_line()
{
  repaint_screen_lines(SCREEN_HEIGHT-1, SCREEN_HEIGHT);
}

void mark_insert_mode_inactive_and_repaint_status()
{
  if (insertMode)
  {
    insertMode = false;
    repaint_status_line();
  }
}

void mark_text_selection_mode_inactive_and_repaint_status()
{
  if (textSelectionModeActive)
  {
    textSelectionModeActive = false;
    repaint_status_line();
  }
}

void clear_selection()
{
  // In all cases when we end up clearing the actively selected
  // content, we also want to disable any active modal text
  // selection mode, if there is any.
  mark_text_selection_mode_inactive_and_repaint_status();
  selectionStartLineNum = selectionEndLineNum = -1;
  selectionStartLine = 0;
}
