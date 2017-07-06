#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "include/argparse/src/argparse.h"
#include "include/except.h"
#include "lib/tc_mem.h"
#include "lib/tc_event.h"
#include "lib/tc_sockets.h"
#include "lib/tc_util.h"
#include "lib/tc_parse_config_file.h"

#ifdef NDEBUG
	#define CONFIG_FILE "GET_HOME" // TODO: how to get home path?
#else
	#define CONFIG_FILE "torchat.sample.conf"
#endif 

static const char *const usages[] = {
    "torchat [options] [[--] args]",
    "torchat [options]",
    NULL,
};

extern Except_T ConfigFileNotFound; 
extern Except_T ConfigFileParseError;

int
main (int argc, char **argv)
{
	int port = -1, torPort = -1;
	// parse config file first
	TRY	
		parse_config_file (CONFIG_FILE, &port, &torPort);
	EXCEPT (ConfigFileNotFound)
		exit_error (ConfigFileNotFound.reason);
	EXCEPT (ConfigFileParseError)
		exit_error (ConfigFileParseError.reason);
	END_TRY;

	// parse command line
	struct argparse_option options[] = {
        OPT_HELP(),
		OPT_INTEGER('p', "port", &port, "bind to this port", 0, 0, 0),
		OPT_INTEGER('t', "torport", &torPort, "port tor daemon is listening to", 0, 0, 0),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, "torchat", usages, 0);
    argparse_describe(&argparse, "TORChat, simple chat over the TOR network.", "\nAdditional description of the program after the description of the arguments.");
    int argcnt = argparse_parse(&argparse, argc, argv);
    if (argparse_requested_help ()) exit (2);
    if (port == -1 || torPort == -1) {
    	// no sufficent paramaters on command line
    	fprintf (stderr, "Usage: %s\nTry 'torchat --help' for more information.\n", usages[0]);
    	exit (2);
    }

	signal (SIGINT, stop_loop);
	autoclosesocket int listenSock = bind_and_listen (port, MAXCONN);
	event_loop (listenSock);
	
	return 0;
}
