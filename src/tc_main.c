#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "lib/mem.h"
#include "lib/tc_event.h"
#include "lib/tc_sockets.h"

int
main (int argc, char **argv)
{
	signal (SIGINT, stop_loop);
	autoclosesocket int listenSock = bind_and_listen (atoi (argv[1]), MAXCONN);
	event_loop (listenSock);
	
	return 0;
}
