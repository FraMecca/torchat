/* $Id: assert.h 6 2007-01-22 00:45:22Z drhanson $ */
#undef assert
#ifdef DEBUG
#include "except.h"
extern void assert(int e);
#define assert(e) ((void)((e)||(RAISE(Assert_Failed),0)))
#else
// remove call to assert
// because not in debug mode
#define assert(e) ((void)0)
#endif
