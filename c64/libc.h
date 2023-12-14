#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>
#include "tick.h"

unsigned char petscii_to_screen_code(unsigned char petscii);

template<typename T>
T MIN(T a, T b)
{
  return a <= b ? a : b;
}

template<typename T>
T MAX(T a, T b)
{
  return a >= b ? a : b;
}

template<typename T>
void qsort(T *arr, u16 n)
{
  T pivot = arr[n/2];
  arr[n/2] = arr[n-1];
  u16 l = 0, r = n-1;
  while(l < r)
    if (arr[l] < pivot) ++l;
    else arr[r] = arr[l], arr[l] = arr[--r];
  arr[r] = pivot;
  if (l > 1)   qsort(arr, l);
  if (r < n-2) qsort(arr+r+1, n-(r+1));
}

#ifdef __mos__
#include "c64_kernal.h"
#endif

#ifndef __mos__

inline void getch() {}

#endif

#ifdef __mos__
#define LINE_SEP '\r'
#define LINE_SEP_STR "\r"
#else
#define LINE_SEP '\n'
#define LINE_SEP_STR "\n"
#endif
