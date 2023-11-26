#pragma once

#include "types.h"

void set_error_squiggles(line_num line_num, line_xpos column, uint8_t type);

void update_error_squiggles_pos();
