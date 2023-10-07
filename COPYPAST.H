#ifndef _COPYPAST_H
#define _COPYPAST_H

void calculate_num_lines_and_chars_selected(unsigned long *lines, unsigned long *chars);

void erase_selected_text();

void copy_selection();
void cut_selection();
void paste_selection();

#endif
