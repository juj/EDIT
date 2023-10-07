#ifndef _HIGHLITE_H
#define _HIGHLITE_H

#define ENABLE_SYNTAX_HIGHLIGHTING 1

struct Line;

#ifdef ENABLE_SYNTAX_HIGHLIGHTING

void clear_highlight(Line far *line);
void syntax_highlight(Line far *line);

#else

inline void clear_highlight(Line far *line) {}
inline void syntax_highlight(Line far *line) {}

#endif

#endif
