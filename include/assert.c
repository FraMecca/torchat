#include "assert.h"
#include "except.h"
const Except_T Assert_Failed = { "Assertion failed" };
void (assert)(int e) {
	assert(e);
}
