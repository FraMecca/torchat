#include "../include/mongoose.h"  // Include Mongoose API definitions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <time.h>
#include "../lib/datastructs.h"
#include <errno.h>
#include "../lib/socks_helper.h"
#include <pthread.h>
#include "../lib/util.h"
#include <signal.h>
#include "../lib/actions.h" // event_routine functions

extern struct data_wrapper *convert_string_to_datastruct (const char *jsonCh); // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper *data); // from json.cpp
extern void log_info (char *json); // from logger.cpp
extern void log_err (char *json); // from logger.cpp
void log_init (char *name, char *verbosity); // from logger.cpp
void log_clear_datastructs (); // from logger.cpp

static bool exitFlag = false; // this flag is set to true when the program should exit

char *HOSTNAME = NULL; // will be read from torrc

static void start_daemon()
{
    /* this function daemonizes the main core of the chat
	 * while in daemon mode, only logging can be used
	 * to monitor the server activity
	 */
#if DEBUG
	printf("The server has been compiled with debug flags, but daemon mode shall not produce any output to stdout.\n");
	printf("The logs can be used to monitor the daemon's behaviour, in particular:\n");
	printf("\t* file.log contains general activity output\n");
	printf("\t* error.log contains only the error output (marked red in file.log)\n");
	printf("\t* debug.log contains a more verbose activity output useful for debugging.\n");
	printf("Also, in case of SIGABRT, SIGINT or SIGSEGV a gdb compatible coredump is produced, containing the current PID of the daemon in its filename.\n");
#endif
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
    if((dir = calloc(200, sizeof(char))) == NULL){
		exit_error("allocation of cwd");
		exit(EXIT_FAILURE);
	}
	getcwd(dir, 200);	
	chdir(dir);
	free(dir);

	/* close all open file descriptors
	 * this is needed in order to stop in/out to/from stdin, stdout, stderr
	 */
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
    return strdup (buf);
}


void
event_routine (struct mg_connection *nc)
{
    /*struct mg_connection *nc = ncV; // nc was casted to void as pthread prototype*/
    struct mbuf *io = &nc->recv_mbuf;
    struct data_wrapper *data;
    char *json; // used to log

    if (io->buf != NULL && io->size > 0) {
        json = calloc (io->size + 1, sizeof (char));
        strncpy (json, io->buf, io->size * sizeof (char));
		json[io->size] = '\0';
        data = convert_string_to_datastruct (io->buf); // parse a datastruct from the message received
        mbuf_remove(io, io->size);      // Discard data from recv buffer
    } else {
        return;
    }
    if (data == NULL) {
        // the json was invalid
        if (json != NULL) {
        	log_err (json);
			free (json);
        // and logged to errlog
        }
        return ;
    }

    switch (data->cmd) {
    	case EXIT :
        	exitFlag = true;
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
        	relay_msg (data);
			// data wrapper is free'd in thread
        	break;
    	case UPDATE:
        	client_update (data, nc);
			free_data_wrapper (data);
        	break;
    	case GET_PEERS :
        	send_peer_list_to_client (data, nc);
			free_data_wrapper (data);
        	break;
    	case HISTORY :
        	// the client asked to receive the history
        	// it specified the id and n lines
        	// (id in json[id], n in json[msg]
        	// send the various messages
			free_data_wrapper (data);
        	break;
		case HOST :
			// the client required the hostname of the server
			// send as a formatted json
			send_hostname_to_client(data, nc, HOSTNAME);
			free_data_wrapper (data);
			break;
    	default:
			free_data_wrapper (data);
        	break;
    }

	if (json != NULL) {
		// should alway be not null
		free (json);
	}
	// data should be freed inside the jump table because it can be used in threads
    return;
}


static void
ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    if (nc->recv_mbuf.size == 0) {
        // can trash
        return;
    }
    // now we just utilize MG_EV_RECV because the response must be send over TOR
    if (ev == MG_EV_RECV) {
        event_routine (nc);
    }
}

int
main(int argc, char **argv)
{
    struct mg_mgr mgr;
    
	if(strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--daemon") == 0) {
        fprintf(stdout, "Starting in daemon mode.\n");
        start_daemon();
    }

#if DEBUG
    signal (SIGSEGV, dumpstack);
    signal (SIGABRT, dumpstack);
    signal (SIGINT, dumpstack);
    log_init ("debug.log", "DEBUG");
#endif
    log_init ("file.log", "INFO");
    log_init ("error.log", "ERROR");

    HOSTNAME = read_tor_hostname ();
    /*pthread_mutex_init (&sem, NULL); // initialize semaphore for log files // sem is in logger.cpp*/

    mg_mgr_init(&mgr, NULL);  // Initialize event manager object

    // Note that many connections can be added to a single event manager
    // Connections can be created at any point, e.g. in event handler function
    if(argc == 2) {
        mg_bind(&mgr, argv[1], ev_handler);  // Create listening connection and add it to the event manager
    } else if (argc == 3) {	// daemon
        mg_bind(&mgr, argv[2], ev_handler);  // Create listening connection and add it to the event manager
    }

    while (!exitFlag) {  // start poll loop
        // stop when the exitFlag is set to false,
        // so mongoose halts and we can collect the threads
        mg_mgr_poll(&mgr, 1000);
    }

    /*wait_all_threads ();*/ /* threads are detached
								so there is no need to wait them and to keep track of them
								but exit with pthread_exit instead of 3 exit in order
								for other threads not to be terminated
							  */
    clear_datastructs (); // free hash table entries
    log_clear_datastructs (); // free static vars in logger.cpp
	destroy_mut(); // free the mutex allocated in list.c
    mg_mgr_free(&mgr); // terminate mongoose connection
    free (HOSTNAME);
    /*pthread_mutex_destroy (&sem);*/
	pthread_exit (0); 			// see above
}
