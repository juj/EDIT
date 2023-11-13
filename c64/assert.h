#pragma once

#define _STDIO_DEBUG_LEVEL 1

#ifndef _STDIO_DEBUG_LEVEL
#define _STDIO_DEBUG_LEVEL 0
#endif

void __assert(bool condition, const char *str, const char *file, int line, const char *func);

#ifdef NDEBUG
#define assert(x) ((void)0)
#elif _STDIO_DEBUG_LEVEL >= 2
#define assert(x) __assert((x), #x, __FILE__, __LINE__, __func__);
#elif _STDIO_DEBUG_LEVEL >= 1
#define assert(x) __assert((x), "", __FILE__, __LINE__, "");
#else
#define assert(x) __assert((x), "", "", __LINE__, "");
#endif
