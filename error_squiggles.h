#pragma once

#include "types.h"

// Return true if most recent build finished without errors (only warnings at most)
bool no_current_compilation_error();

void set_error_squiggles(line_num line_num, line_xpos column, uint8_t type);
void hide_error_squiggles();

void update_error_squiggles_pos();
