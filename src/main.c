#include "../include/mongoose.h"  // Include Mongoose API definitions
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> // umask for daemonization
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
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

static pthread_mutex_t sem; // semaphore that will be used for race conditions on logfiles
// would be more correct if you have a semaphore for every different file that could be opened

char *HOSTNAME = NULL; // will be read from torrc

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
    // switch to a new thread and do everything in that thread
    /*the thread handles the connection and works on that.*/

    // now we just utilize MG_EV_RECV because the response must be send over TOR
    if (ev == MG_EV_RECV) {
        /*pthread_t *th = NULL;*/
        /*// allocate th*/
        /*if ((th = malloc (sizeof (pthread_t))) == NULL) {*/
        /*exit_error ("malloc: th: ");*/
        /*}*/
        /*case MG_EV_RECV:*/
        event_routine (nc);
        /*keep_track_of_threads (th);*/
    }
}

int
main(int argc, char **argv)
{
    struct mg_mgr mgr;
    
	if(strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--daemon") == 0) {
        fprintf(stdout, "Starting in daemon mode.\n");
        daemon(0, 0);
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
    pthread_mutex_init (&sem, NULL); // initialize semaphore for log files

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
	destroy_mut(); // free the mutex allocated in list.c
    clear_datastructs (); // free hash table entries
    log_clear_datastructs (); // free static vars in log.c
    mg_mgr_free(&mgr); // terminate mongoose connection
    free (HOSTNAME);
    pthread_mutex_destroy (&sem);
	pthread_exit (0); 			// see above
}
