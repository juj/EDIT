#ifndef _COPYPAST_H
#define _COPYPAST_H

#include "types.h"

void calculate_num_lines_and_chars_selected(line_num *lines, file_idx *chars);

void erase_selected_text();

void copy_selection();
void cut_selection();
void paste_selection();

#endif
