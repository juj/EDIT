#ifndef _IASSERT_H
#define _IASSERT_H

#ifdef __mos__
#define assert(x) ((void)0)
#else
#include <assert.h>
#endif

#endif
