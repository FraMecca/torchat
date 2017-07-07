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
#include "lib/tc_handle.h"
#include "lib/tc_conffile.h"

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
#ifndef NDEBUG
	signal (SIGSEGV, dumpstack);
	signal (SIGABRT, dumpstack);
	/*log_init ("debug.log", "DEBUG");*/
#endif

	int port = -1, torPort = -1;
	autofree char *torDomain = NULL;

	// parse config file first
	TRY	
		parse_config_file (CONFIG_FILE, &port, &torPort, &torDomain);
	EXCEPT (ConfigFileNotFound)
		exit_error (ConfigFileNotFound.reason);
	EXCEPT (ConfigFileParseError)
		exit_error (ConfigFileParseError.reason);
	END_TRY;

	// parse command line
	struct argparse_option options[] = {
        OPT_HELP(),
		OPT_INTEGER('p', "port", 		&port, 		"bind to this port", 0, 0, 0),
		OPT_INTEGER('t', "torport", 	&torPort, 	"port tor daemon is listening to", 0, 0, 0),
		OPT_STRING ('D', "domain" , 	&torDomain, "hidden .onion address", 0, 0, 0),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, "torchat", usages, 0);
    argparse_describe(&argparse, "TORChat, simple chat over the TOR network.", "");
    int ra = argparse_parse(&argparse, argc, argv);
    if (ra == -1 || argparse_requested_help ()) exit (2); // --help on cli, exit
    if (port == -1 || torPort == -1 || torDomain == NULL) {
    	// no sufficent paramaters on command line
    	fprintf (stderr, "Usage: %s\nTry 'torchat --help' for more information.\n", usages[0]);
    	exit (2);
    }

	connect_to_tor ("localhost", torPort);

	signal (SIGINT, stop_loop);
	autoclosesocket int listenSock = bind_and_listen (port, MAXCONN);
	event_loop (listenSock);

	destroy_proxy_connection ();
	tc_destroy_handlers ();
	
	return 0;
}
