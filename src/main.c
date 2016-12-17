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
#include <syslog.h>
extern struct data_wrapper convert_string_to_datastruct (const char *jsonCh); // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper *data); // from json.cpp

static bool exitFlag = false; // this flag is set to true when the program should exit

bool
relay_msg (struct data_wrapper*);
void 
log_msg (char *id, char *msg, enum command c);

static pthread_mutex_t sem; // semaphore that will be used for race conditions on logfiles
// would be more correct if you have a semaphore for every different file that could be opened

static char *HOSTNAME = NULL; // will be read from torrc

static void skeleton_daemon(char *dir)
{
	/*dir is the current working directory*/
    /*this function daemonizes the main core of the chat*/
	pid_t pid;
    int x;
  
    /* Fork off the parent process */
    pid = fork();
    /* An error occurred */
    if (pid < 0){
        exit(EXIT_FAILURE);
        } else if(pid > 0) {
        exit(EXIT_SUCCESS); // the parent exites
        }
    /* On success: The child process becomes session leader */
    if (setsid() < 0){
        exit(EXIT_FAILURE);
     	}
    /* Catch, ignore and handle signals */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    /* Fork off for the second time, it is good practice*/
    pid = fork();

    /* An error occurred */
    if (pid < 0){
        exit(EXIT_FAILURE);
        } else if(pid > 0){
        exit(EXIT_SUCCESS); // the parent exites again
     }
    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the current directory
	 * note that this might not be necessary, safety reason
	 */
    chdir(dir); 
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
*event_routine (void *ncV) 
{
	struct mg_connection *nc = ncV; // nc was casted to void as pthread prototype
  	struct mbuf *io = &nc->recv_mbuf;
	struct data_wrapper *data = calloc (1, sizeof (struct data_wrapper));
	char *msg;

  	if (io->buf != NULL) {
		*data = convert_string_to_datastruct (io->buf); // parse a datastruct from the message received
		mbuf_remove(io, io->len);      // Discard data from recv buffer
	} else { 
		return 0;
	}
	if (data->msg == NULL) {
		// the json was invalid 
		//
		// and logged to errlog
		return 0;
	}

	switch (data->cmd) {
		case EXIT :
			exitFlag = true;
			break;
		case RECV :
			log_msg (data->id, data->msg, data->cmd); // first log then frog
			if (!peer_exist (data->id)) {
				// guess what, peer doesn't exist
				insert_peer (data->id);
				// insert in hash tables only peer that sent RECV, not other cmd
			}
			insert_new_message (data->id, data->msg);
      	  	break;
      	case SEND :
      		// mongoose is told that you want to send a message to a peer
      	  	log_msg (data->id, data->msg, data->cmd);
      	  	relay_msg (data);
      	  	break;
		case UPDATE:
			// the client asks for unread messages from data->id peer
			// get the OLDEST, send as a json
			// this is supposed to be executed periodically
			// by the client
			/*free(data->msg);*/
			strncpy(data->id, data->msg,29*sizeof(char));
			data->id[strlen(data->id)] = '\0';
			// if no msg, get_unread_message should return NULL
			if((msg = get_unread_message(data->msg)) != NULL){
				// now we convert the message in a json and send it
				free (data->msg);
				data->msg = msg;
				char *unreadMsg = convert_datastruct_to_char(data);
				mg_send (nc, unreadMsg, strlen(unreadMsg));
			} else {
				data->cmd = END;
				char *unreadMsg = convert_datastruct_to_char(data);
				mg_send (nc, unreadMsg, strlen(unreadMsg));
			}
			break;
		case GET_PEERS :
			// the client asked to receive the list of all the peers that send the server a message (not read yet)
			// send the list as a parsed json, with peer field comma divided
			// the char containing the list of peers commadivided is then put into a json 
			// just store it in data.msg
			free (data->msg);
			data->msg = get_peer_list ();
			if (data->msg == NULL) {
				data->msg = strdup ("");
				// needed if no peers messaged us
			}
			char *response = convert_datastruct_to_char (data);
			if (nc->iface != NULL) {
				// if iface is not null the client is waiting for response
				mg_send (nc, response, strlen (response));
			}

			break;
		default:
      		return 0;
    }
    free (data->msg);
    free (data);
    return 0; // pthread_exit()
}


static void
ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
  	pthread_t *th = NULL;
  	// allocate th
  	if ((th = malloc (sizeof (pthread_t))) == NULL) {
  		exit_error ("malloc: th: ");
  	}
    // switch to a new thread and do everything in that thread
    /*the thread handles the connection and works on that.*/

	// now we just utilize MG_EV_RECV because the response must be send over TOR
  	if (ev == MG_EV_RECV) {
    	/*case MG_EV_RECV:*/
    	pthread_create (th, NULL, event_routine, nc);
    	keep_track_of_threads (th);
  	}
}

void
log_msg (char *onion, char *msg, enum command MODE)
{
	// use syslog to log
	openlog (onion, LOG_PID|LOG_CONS, LOG_USER);
	syslog (LOG_INFO, "%s", msg);
	closelog ();
}

bool
relay_msg (struct data_wrapper *data)
{
	char id[30];
      	// first change command to RECV, not SEND
    data->cmd = RECV;
	strcpy (id, data->id); // save dest address
	strcpy (data->id, HOSTNAME); // copy your hostname to the field
	data->date = get_short_date();
	char *msg = convert_datastruct_to_char (data);
	bool ret = send_over_tor (id, data->portno, msg, 9250);
	free (msg);
	return ret;
}

int
main(int argc, char **argv) {
  	struct mg_mgr mgr;
  	char cwd[128];
  	if(strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--daemon") == 0){
	  	fprintf(stdout, "Starting in daemon mode.\n");
	  	if(getcwd(cwd, sizeof cwd) != NULL){
	  		skeleton_daemon(cwd);
	  	} else {
	  		perror("getcwd");
	  	}
  	}

  	HOSTNAME = read_tor_hostname ();
  	pthread_mutex_init (&sem, NULL); // initialize semaphore for log files

  	mg_mgr_init(&mgr, NULL);  // Initialize event manager object

  	// Note that many connections can be added to a single event manager
  	// Connections can be created at any point, e.g. in event handler function
  	if(argc == 2){
  		mg_bind(&mgr, argv[1], ev_handler);  // Create listening connection and add it to the event manager
  	} else if (argc == 3){	// daemon
  		mg_bind(&mgr, argv[2], ev_handler);  // Create listening connection and add it to the event manager
  	}

  	while (!exitFlag) {  // start poll loop
  	  	// stop when the exitFlag is set to false,
  	  	// so mongoose halts and we can collect the threads
      	mg_mgr_poll(&mgr, 1000);
  	}

	wait_all_threads (); 
  	mg_mgr_free(&mgr);
  	free (HOSTNAME);
  	pthread_mutex_destroy (&sem);
  	return 0;
}
