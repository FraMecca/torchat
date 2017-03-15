/* $Id: ut_assert.h 6 2007-01-22 00:45:22Z drhanson $ */
#undef ut_assert
#ifdef NDEBUG
// remove call to ut_assert
// because not in debug mode
#define ut_assert(e) ((void)0)
#else
#include "except.h"
extern void ut_assert(int e);
#define ut_assert(e) ((void)((e)||(RAISE(Assert_Failed),0)))
#endif
