#include "../include/mongoose.h"  // Include Mongoose API definitions
#include <stdio.h>
#include <stdlib.h>
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
extern struct data_wrapper convert_string_to_datastruct (const char *jsonCh); // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper data); // from json.cpp

bool
relay_msg (const struct data_wrapper);
void 
log_msg (char *id, char *msg, enum command c);

static pthread_mutex_t sem; // semaphore that will be used for race conditions on logfiles
// would be more correct if you have a semaphore for every different file that could be opened

static char *HOSTNAME = NULL; // will be read from torrc

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
*event_routine (void *voidIo) 
{
	struct mbuf *io = voidIo; // io was casted to void as pthread prototype
  	struct data_wrapper *data = calloc (1, sizeof (struct data_wrapper));
  	if (io->buf != NULL) {
		*data = convert_string_to_datastruct (io->buf); // parse a datastruct from the message received
		mbuf_remove(io, io->len);      // Discard data from recv buffer
	} else { 
		return 0;
	}
	if (data->msg == NULL) {
		// the json was invalid 
		// and logged to errlog
		return 0;
	}

	switch (data->cmd) {
		case EXIT :
			exit (0);
			break;
		case RECV :
			log_msg (data->id, data->msg, data->cmd); // first log then frog
			if (!peer_exist (data->id)) {
				// guess what, peer doesn't exist
				insert_peer (data->id);
			}
			insert_new_message (data->id, data->msg);
      	  	break;
      	case SEND :
      		// mongoose is told that you want to send a message to a peer
      	  	log_msg (data->id, data->msg, data->cmd);
      	  	relay_msg (*data);
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
  	struct mbuf *io = &nc->recv_mbuf;
  	pthread_t th;
	
	// now we just utilize MG_EV_RECV because the response must be send over TOR
  	if (ev == MG_EV_RECV) {
    	/*case MG_EV_RECV:*/
    	// switch to a new thread and do everything in that thread
    	pthread_create (&th, NULL, event_routine, io);

  	}
}

void
log_msg (char *onion, char *msg, enum command MODE)
{
	FILE *fp;
	char *id = strdup ("?");

	if (MODE == RECV) {
		id = strdup (onion);
	} else if (MODE == SEND) {
		id = strdup ("YOU");
	}
	fp = fopen (onion, "a");
	if (fp == NULL) {
		exit_error ("fopen");
	}
	pthread_mutex_lock (&sem);
	fprintf (fp, "{[%s] | [%s]}:\t%s\n", get_date (), id, msg);
	fclose (fp);
	pthread_mutex_unlock (&sem);
	free (id);
}


bool
relay_msg (struct data_wrapper data)
{
	char id[30];
      	// first change command to RECV, not SEND
    data.cmd = RECV;
	strcpy (id, data.id); // save dest address
	strcpy (data.id, HOSTNAME); // copy your hostname to the field
	char *msg = convert_datastruct_to_char (data);
	bool ret = send_over_tor (id, data.portno, msg, 9250);
	free (msg);
	return ret;
}


int
main(int argc, char **argv) {
  struct mg_mgr mgr;
  signal (SIGUSR1, exit);

  HOSTNAME = read_tor_hostname ();
  pthread_mutex_init (&sem, NULL); // initialize semaphore for log files

  mg_mgr_init(&mgr, NULL);  // Initialize event manager object

  // Note that many connections can be added to a single event manager
  // Connections can be created at any point, e.g. in event handler function
  mg_bind(&mgr, argv[1], ev_handler);  // Create listening connection and add it to the event manager

  while (true) {  // Start infinite event loop
      mg_mgr_poll(&mgr, 1000);
  }

  mg_mgr_free(&mgr);
  free (HOSTNAME);
  pthread_mutex_destroy (&sem);
  return 0;
}
