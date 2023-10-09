#ifndef _STATE_H
#define _STATE_H

#include <stdint.h>
#include "memory.h"
#include "types.h"

struct File;
struct Line;

extern allocated_mem_size_t allocatedMemory;

// Global editor state
extern File *currentFile;
extern Line far *firstDisplayedLine; // the topmost line that the view is scrolled to
extern line_num firstDisplayedLineNumber;
extern Line far *currentLine;

// The desired X coordinate, which might not be reachable if the
// current line is too short.
extern line_xpos viewportXOffset;
extern line_xpos virtualX;
extern line_xpos cursorX;
extern cursor_pos cursorY;

// If true, character insert mode is enabled.
extern bool insertMode;

// If true, text selection mode is active, so cursor keys select text as the cursor is moved.
// This is used on platforms that don't have Shift-text select support.
extern bool textSelectionModeActive;

extern bool showWhitespace;

// 32: spaces as indentation, 9: tab chars as indentation
extern bool tabModeIsSpaces;
extern char tabChar;

// number of chars for indentation.
extern line_xpos indentSize;

extern unsigned char editorTextColor;

void clear_selection();

extern long selectionStartLineNum, selectionEndLineNum;
extern Line far *selectionStartLine; // if 0, no active selection
extern Line far *selectionEndLine;
extern long selectionStartColumn, selectionEndColumn;

long current_line_number();

// Returns zero-based current coordinate of the cursor on current line.
// If 0, cursor is at start of line. Max value == line->length.
long current_column();

#endif
