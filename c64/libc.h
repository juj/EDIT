#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
