#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

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

void print64(u64 x);

#ifndef __mos__

inline void getch() {}

#endif
