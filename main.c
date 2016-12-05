#include "mongoose.h"  // Include Mongoose API definitions
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
#include "datastructs.h"
#include <errno.h>
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


static void
ev_handler(struct mg_connection *nc, int ev, void *ev_data) {

  	struct mbuf *io = &nc->recv_mbuf;

  	if (ev == MG_EV_RECV) {
    	/*case MG_EV_RECV:*/
  	  	struct data_wrapper data = convert_string_to_datastruct (io->buf);

      	if (data.cmd == RECV) {
      	  	printf ("ricevuto %s da %s\n", data.msg, data.id);
      	} else if (data.cmd == SEND) {
      		// first change command to RECV, not SEND
      		printf ("Provo a mandare %s a %s\n", data.msg, data.id);
      		data.cmd = RECV;
      	  	relay_msg (data);
      	  	log_msg (data.id, data.msg);
      	}

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
relay_msg (const struct data_wrapper data)
{
	int sockfd,  n;
	struct sockaddr_in address;
	struct hostent *server;

	// first open a socket to destination ip
	sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		exit_error ("socket");
	}
	server = gethostbyname (data.id);
	address.sin_family = AF_INET;
	bcopy ((char *)server->h_addr_list[0], &address.sin_addr.s_addr, server->h_length);
	address.sin_port = htons (data.portno);
	if (connect (sockfd, (struct sockaddr *) &address, sizeof (address)) < 0) {
		exit_error ("socket");
	}
	char *msg = convert_datastruct_to_char (data);
	write (sockfd, msg, strlen (msg));
	close (sockfd);
	return true;
}


int
main(int argc, char **argv) {
  struct mg_mgr mgr;

  mg_mgr_init(&mgr, NULL);  // Initialize event manager object

  // Note that many connections can be added to a single event manager
  // Connections can be created at any point, e.g. in event handler function
  mg_bind(&mgr, argv[1], ev_handler);  // Create listening connection and add it to the event manager

  for (;;) {  // Start infinite event loop
      mg_mgr_poll(&mgr, 1000);
  }

  mg_mgr_free(&mgr);
  return 0;
}
