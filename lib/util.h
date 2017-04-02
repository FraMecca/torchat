#pragma once

#include <pthread.h>

// This header contains utils common to every .c file
#define GOTHERE fprintf(stdout, "%s:%s:%d\n", __FILE__, __func__, __LINE__)

void
exit_error (char *s);

char *
get_date ();

char *
get_short_date();

void
dumpstack ();
