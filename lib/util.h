#pragma once

#include <pthread.h>

// This header contains utils common to every .c file

void
exit_error (char *s);

char *
get_date ();

char *
get_short_date();

void
set_thread_detached (pthread_attr_t *attr);

void
dumpstack ();
