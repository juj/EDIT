#ifndef _STATE_H
#define _STATE_H

class File;
struct Line;

extern long allocatedMemory;

// Global editor state
extern File *currentFile;
extern Line far *firstDisplayedLine; // the topmost line that the view is scrolled to
extern unsigned long firstDisplayedLineNumber;
extern int viewportXOffset; // TODO: implement
extern Line far *currentLine;

// The desired X coordinate, which might not be reachable if the
// current line is too short.
extern int virtualX;

extern int cursorX;
extern int cursorY;

// If true, character insert mode is enabled.
extern int insertMode;

extern int showWhitespace;

// 32: spaces as indentation, 9: tab chars as indentation
extern char tabMode;
extern char tabChar;

// number of chars for indentation.
extern int indentSize;

#define FILE_MODIFIED() currentFile->hasUnsavedChanges = 1

extern unsigned char editorTextColor;

void clear_selection();

extern unsigned long selectionStartLineNum, selectionEndLineNum;
extern Line far *selectionStartLine; // if 0, no active selection
extern Line far *selectionEndLine;
extern unsigned long selectionStartColumn, selectionEndColumn;

unsigned long current_line_number();
unsigned long current_column();

#endif
