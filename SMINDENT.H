#ifndef _SMINDENT_H
#define _SMINDENT_H

#include "memory.h"

struct Line;

/*
int gcd(int a, int b);
int guess_space_indent_amount(int prevLine, int curLine);
int infer_indent_gcd(Line far *line);
*/

void insert_indent();
void unindent();

#endif
