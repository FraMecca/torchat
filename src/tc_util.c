#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

void
exit_error (const char *st)
{
	fprintf (stderr, "%s\n", st);
	exit (EXIT_FAILURE);
}

void
exit_on_stall (int signum)
{
	fprintf (stderr, "Stalled, exiting abruptly\n");
	_Exit (EXIT_FAILURE);
}

void

dumpstack(int sig)
{
	//use gcore to generate a coredump
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
