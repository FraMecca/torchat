#include "ut_assert.h"
#include "except.h"
const Except_T Assert_Failed = { "Assertion failed" };
void (ut_assert)(int e) {
	ut_assert(e);
}
