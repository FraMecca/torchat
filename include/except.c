#include <stdlib.h>
#include <stdio.h>
#include "ut_assert.h"
#include "except.h"
#define T Except_T
Except_Frame *Except_stack = NULL;
void Except_raise(const T *e, const char *file,
	int line) {
#ifdef WIN32
	Except_Frame *p;

	if (Except_index == -1)
		Except_init();
	p = TlsGetValue(Except_index);
#else
	Except_Frame *p = Except_stack;
#endif
	ut_assert(e);
	if (p == NULL) {
		fprintf(stderr, "Uncaught exception");
		if (e->reason)
			fprintf(stderr, " %s", e->reason);
		else
			fprintf(stderr, " at 0x%p", e);
		if (file && line > 0)
			fprintf(stderr, " raised at %s:%d\n", file, line);
		fprintf(stderr, "aborting...\n");
		fflush(stderr);
		abort();
	}
	p->exception = e;
	p->file = file;
	p->line = line;
#ifdef WIN32
	Except_pop();
#else
	Except_stack = Except_stack->prev;
#endif
	longjmp(p->env, Except_raised);
}
#ifdef WIN32
_CRTIMP void __cdecl _ut_assert(void *, void *, unsigned);
#undef ut_assert
#define ut_assert(e) ((e) || (_ut_assert(#e, __FILE__, __LINE__), 0))

int Except_index = -1;
void Except_init(void) {
	BOOL cond;

	Except_index = TlsAlloc();
	ut_assert(Except_index != TLS_OUT_OF_INDEXES);
	cond = TlsSetValue(Except_index, NULL);
	ut_assert(cond == TRUE);
}

void Except_push(Except_Frame *fp) {
	BOOL cond;

	fp->prev = TlsGetValue(Except_index);
	cond = TlsSetValue(Except_index, fp);
	ut_assert(cond == TRUE);
}

void Except_pop(void) {
	BOOL cond;
	Except_Frame *tos = TlsGetValue(Except_index);

	cond = TlsSetValue(Except_index, tos->prev);
	ut_assert(cond == TRUE);
}
#endif
