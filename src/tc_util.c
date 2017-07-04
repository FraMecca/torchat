#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void
exit_error (char *st)
{
	fprintf (stderr, "%s\n", st);
	exit (EXIT_FAILURE);
}
