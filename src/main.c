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
#include "socks_helper.h"
extern struct data_wrapper convert_string_to_datastruct (const char *jsonCh); // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper data);

static void
exit_error (char *s)
{
	perror (s);
	exit (-1);
}


bool
relay_msg (const struct data_wrapper);

bool
log_msg (char *id, char *msg);

static char HOSTNAME[] = "7a73izkph3wutuh6.onion" ;

static void
ev_handler(struct mg_connection *nc, int ev, void *ev_data) {

  	struct mbuf *io = &nc->recv_mbuf;
	
	// now we just utilize MG_EV_RECV because the response must be send over TOR
  	if (ev == MG_EV_RECV) {
    	/*case MG_EV_RECV:*/
  	  	struct data_wrapper *data = calloc (1, sizeof (struct data_wrapper));
  	  	*data = convert_string_to_datastruct (io->buf); // parse a datastruct from the message received

		switch (data->cmd) {
			case EXIT :
				exit (0);
				break;
			case RECV :
      	  		printf ("ricevuto %s da %s\n", data->msg, data->id); // a peer just messaged you
      	  		break;
      	  	case SEND :
      			// mongoose is told that you want to send a message to a peer
  	  
      			printf ("%s\n", convert_datastruct_to_char (*data));
      	  		relay_msg (*data);
      	  		log_msg (data->id, data->msg);
      	}
      	free (data);

  	}
  	mbuf_remove(io, io->len);      // Discard data from recv buffer
}

bool
log_msg (char *id, char *msg)
{
	FILE *fp;
	time_t t = time (NULL);
	struct tm *tm = localtime (&t);
	char date[50];

	fp = fopen (id, "a");
	if (fp == NULL) {
		exit_error ("fopen");
	}
	strcpy (date, asctime (tm));
	date[strlen (date)-1] = '\0';
	fprintf (fp, "{[%s] | [%s]}:\t%s\n", date, id, msg);
	fclose (fp);
	return true;
}


bool
relay_msg (struct data_wrapper data)
{
	char id[30];
      	// first change command to RECV, not SEND
    data.cmd = RECV;
	strcpy (id, data.id); // save dest address
	strcpy (data.id, HOSTNAME);
	return send_over_tor (id, data.portno, convert_datastruct_to_char (data), 9250);
}


int
main(int argc, char **argv) {
  struct mg_mgr mgr;
  signal (SIGUSR1, exit);

  mg_mgr_init(&mgr, NULL);  // Initialize event manager object

  // Note that many connections can be added to a single event manager
  // Connections can be created at any point, e.g. in event handler function
  mg_bind(&mgr, argv[1], ev_handler);  // Create listening connection and add it to the event manager

  while (true) {  // Start infinite event loop
      mg_mgr_poll(&mgr, 1000);
  }

  mg_mgr_free(&mgr);
  return 0;
}
