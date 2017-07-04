#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "include/argparse/src/argparse.h"
#include "lib/mem.h"
#include "lib/tc_event.h"
#include "lib/tc_sockets.h"

static const char *const usages[] = {
    "torchat [options] [[--] args]",
    "torchat [options]",
    NULL,
};

int
main (int argc, char **argv)
{
	int port = -1;
	struct argparse_option options[] = {
        OPT_HELP(),
		OPT_INTEGER('p', "port", &port, "bind to this port", 0, 0, 0),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, "torchat", usages, 0);
    argparse_describe(&argparse, "TORChat, simple chat over the TOR network.", "\nAdditional description of the program after the description of the arguments.");
    int argcnt = argparse_parse(&argparse, argc, argv);
    if (argparse_requested_help ()) exit (2);
    if (argcnt == 0 || 
    		port == -1)
    {
    	fprintf (stderr, "Usage: %s\nTry 'torchat --help' for more information.\n", usages[0]);
    	exit (2);
    }


	signal (SIGINT, stop_loop);
	autoclosesocket int listenSock = bind_and_listen (port, MAXCONN);
	event_loop (listenSock);
	
	return 0;
}
