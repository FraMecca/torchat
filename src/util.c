#include "lib/util.h"
#include <stdlib.h> // exit
#include <errno.h> // perror
#include <time.h> // localtime
#include <string.h>
#include <stdio.h> // perror
#include <signal.h> // sigsegv sigabrt
#include <unistd.h> // getpid

void
exit_error (char *s)
{
	// send perror and exit
	perror (s);
	exit (1);
}

char *
get_date ()
{
	// get current date
	time_t t = time (NULL);
	struct tm *tm = localtime (&t);
	char date[50];
	strcpy (date, asctime (tm));
	date[strlen (date)-1] = '\0'; // insert linetermination
	return strdup (date);
}

char *
get_short_date ()
{
	time_t t = time (NULL);
	struct tm *tm = localtime (&t);
	char date[50] = {0};
	strftime(date, 8, "%H:%M", tm);
	return strdup((char*)date);
}

void 
dumpstack(int sig) {
	// use gcore to generate a coredump
	char sys[160];

	sprintf(sys, "echo 'where\ndetach' | gcore -o torchat_coredump %d", getpid());
	system(sys);
	// core has been dumped
	if (sig == SIGSEGV) {
		exit (139);
	} else if (sig == SIGINT) {
		exit (130);
	} else {
		// sig == SIGABRT
		exit (134);
	}
}
