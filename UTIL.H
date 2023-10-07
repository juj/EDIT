#ifndef _UTIL_H
#define _UTIL_H

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define ABS(x) ((x) >= 0 ? (x) : (-(x)))

void log_x(const char *str);

void NOTIMPL(const char *x);

#endif
