#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define ABS(x) ((x) >= 0 ? (x) : (-(x)))

void log_x(const char *str);

void NOTIMPL(const char *x);

// Returns min(a+b,255)
uint8_t ADD_U8_SAT(uint8_t a, uint8_t b);
// Returns max(a-b, 0)
uint8_t SUB_U8_SAT(uint8_t a, uint8_t b);

#endif
