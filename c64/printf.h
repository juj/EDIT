#pragma once

#include <stdio.h>//xxxx
#include <stdarg.h>
#include "c64_kernal.h"

// Prints the given uint16 to CHROUT.
void chrout_u16(uint16_t num);

void Printf(const char *format, ...);