#ifndef _VIEWPORT_H
#define _VIEWPORT_H

struct File;

void open_file_in_viewport(File *file, unsigned char bringToMruFront);

void run_alt_tab_menu();

void switch_to_next_file(bool nextOrPrev);

#endif

