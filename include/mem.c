#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "ut_assert.h"
#include "except.h"
#include "mem.h"
const Except_T Mem_Failed = { "Allocation Failed" };
void *Mem_alloc(long nbytes, const char *file, int line){
	void *ptr;
	ut_assert(nbytes > 0);
	ptr = malloc(nbytes);
	if (ptr == NULL)
		{
			if (file == NULL)
				RAISE(Mem_Failed);
			else
				Except_raise(&Mem_Failed, file, line);
		}
	return ptr;
}
void *Mem_calloc(long count, long nbytes,
	const char *file, int line) {
	void *ptr;
	ut_assert(count > 0);
	ut_assert(nbytes > 0);
	ptr = calloc(count, nbytes);
	if (ptr == NULL)
		{
			if (file == NULL)
				RAISE(Mem_Failed);
			else
				Except_raise(&Mem_Failed, file, line);
		}
	return ptr;
}
void Mem_free(void *ptr) {
	if (ptr)
		free(ptr);
}
void *Mem_resize(void *ptr, long nbytes,
	const char *file, int line) {
	ut_assert(ptr);
	ut_assert(nbytes > 0);
	ptr = realloc(ptr, nbytes);
	if (ptr == NULL)
		{
			if (file == NULL)
				RAISE(Mem_Failed);
			else
				Except_raise(&Mem_Failed, file, line);
		}
	return ptr;
}
char *Mem_strdup(const char *s, const char *file, int line){
	char *ptr;
	ptr = strdup (s);
	if (ptr == NULL)
		{
			if (file == NULL)
				RAISE(Mem_Failed);
			else
				Except_raise(&Mem_Failed, file, line);
		}
	return ptr;
}
