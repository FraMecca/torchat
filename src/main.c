#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h> //umask
#include <assert.h> // assert
#include "include/mem.h" // CALLOC and MALLOC
#include "lib/datastructs.h"
#include "lib/socks_helper.h"
#include "lib/util.h"
#include "lib/actions.h" // event_routine functions
#include "include/libdill.h"

extern struct data_wrapper *convert_string_to_datastruct (const char *jsonCh); // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper *data); // from json.cpp
extern void log_info (char *json); // from logger.cpp
extern void log_err (char *json); // from logger.cpp
void log_init (char *name, char *verbosity); // from logger.cpp
void log_clear_datastructs (); // from logger.cpp

static bool exitFlag = false; // this flag is set to true when the program should exit

char *HOSTNAME = NULL; // will be read from torrc

static void
start_daemon()
{
    /* this function daemonizes the main core of the chat
	 * while in daemon mode, only logging can be used
	 * to monitor the server activity
	 */
    pid_t pid;
    int x;
	char *dir;

    /* Fork off the parent process */
    pid = fork();
    /* An error occurred */
    if (pid < 0) {
        exit(EXIT_FAILURE);
    } else if(pid > 0) {
        exit(EXIT_SUCCESS); // the parent exites
    }
    /* On success: The child process becomes session leader */
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }
    /* Catch, ignore and handle signals */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time, so that the daemon does not become session leader*/
    pid = fork();
    /* An error occurred */
    if (pid < 0) {
        exit(EXIT_FAILURE);
    } else if(pid > 0) {
        exit(EXIT_SUCCESS); // the parent exites again
    }

    /* Set new file permissions */
    umask(0);

	/* Change the working directory to the current one*/
    dir = CALLOC (200, sizeof(char));
	getcwd(dir, 200);	
	chdir(dir);
	free(dir);

	/*close all open file descriptors*/
	/*this is needed in order to stop in/out to/from stdin, stdout, stderr*/
	for (x = sysconf(_SC_OPEN_MAX); x>=0; x--) {
		close (x);
	}

}

char *
read_tor_hostname (void)
{
    // still hardcoded
    // opens ../tor/hostname
    FILE *fp = fopen ("tor/hostname", "r");
    if (fp == NULL) {
        exit_error ("fopen: torrc:");
    }
    char buf[50];
    fscanf (fp, "%s", buf);
    fclose (fp);
    return STRDUP (buf);
}

coroutine void
event_routine (const int sock) 
{
	// manage server RECV events
	struct data_wrapper *data = NULL;
	char *json = NULL;
    int64_t deadline = now() + 120000; // deadline in ms, 2min
	// sock is between the daemon and the client
	// torSock is between the daemon and TOR (other peer)
	int torSock = 0;

	while (parse_connection (sock, &data, &json, deadline) > 0) {
			// keep connection open with client till deadline
			// then exit coroutine
		if (torSock == 0){
			torSock = handshake_with_tor (9250);
		}
    	switch (data->cmd) {
    		case EXIT :
        		exitFlag = true;
        		log_info (json);
				free_data_wrapper (data);
        		break;
    		case RECV :
        		log_info (json); // first log
        		store_msg (data);
				free_data_wrapper (data);
        		break;
    		case SEND :
        		// mongoose is told that you want to send a message to a peer
        		log_info (json);
        		relay_msg (data, sock, torSock, deadline);
				free_data_wrapper (data);
				// data wrapper is free'd in routine 
        		break;
    		case UPDATE:
        		client_update (data, sock, deadline);
				free_data_wrapper (data);
        		break;
    		case GET_PEERS :
        		send_peer_list_to_client (data, sock, deadline);
				free_data_wrapper (data);
        		break;
			case HOST :
				// the client required the hostname of the server
				// send as a formatted json
				send_hostname_to_client(data, sock, HOSTNAME, deadline);
				free_data_wrapper (data);
				break;
    		default:
				free_data_wrapper (data);
        		break;
    	}
		FREE (json);
		// data should be freed inside the jump table because it can be used in threads
	}
	// cleanup
    int rc = hclose(sock);
	fdclean(torSock);
    assert(rc == 0);
    return;
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf (stdout, "USAGE...\n");
		exit (EXIT_FAILURE);
	}
	int port;
	if(strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--daemon") == 0) {
        start_daemon();
    	port = atoi (argv[2]);
    } else {
    	port = atoi (argv[1]);
	}

#ifndef NDEBUG
    signal (SIGSEGV, dumpstack);
    signal (SIGABRT, dumpstack);
    signal (SIGINT, dumpstack);
    log_init ("debug.log", "DEBUG");
#endif
    log_init ("file.log", "INFO");
    log_init ("error.log", "ERROR");

	// initialization of datastructs
    HOSTNAME = read_tor_hostname ();
	/*initialize_fileupload_structs ();*/
	
    struct ipaddr addr;
    int rc = ipaddr_local(&addr, NULL, port, 0);
    assert(rc == 0);
    int ls = tcp_listen(&addr, 10);
    if(ls < 0) {
        exit_error("Can't open listening socket");
    }

    while (!exitFlag) {  // start poll loop
        // stop when the exitFlag is set to false,
        int sock = tcp_accept(ls, NULL, -1);
        // check errno for tcp_accept
        assert(sock >= 0);
        sock  = crlf_attach(sock); // clrf means that lines must be \r\n terminated
        assert(sock >= 0);
        int cr = go(event_routine(sock));
        assert(cr >= 0);
    }

    /*destroy_fileupload_structs ();*/
    clear_datastructs (); // free hash table entries
    log_clear_datastructs (); // free static vars in logger.cpp
 	destroy_mut(); // free the mutex allocated in datastruct.c
    if (HOSTNAME != NULL) {
    	FREE (HOSTNAME);
    }
    exit (EXIT_SUCCESS);
}
