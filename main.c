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
extern struct data_wrapper convert_string_to_datastruct (const char *jsonCh); // from json.cpp


bool
send_msg (char *id, int portno, char *msg);

bool
log_msg (char *id, char *msg);


static void
ev_handler(struct mg_connection *nc, int ev, void *ev_data) {

  struct mbuf *io = &nc->recv_mbuf;
      char *id;
      struct data_wrapper data;

  switch (ev) {
    case MG_EV_RECV:
      data = convert_string_to_datastruct (io->buf);

      if (data.cmd == RECV) {
      	  printf ("ricevuto %s da %s\n", data.id, data.msg);
      } else if (data.cmd == SEND) {
      	  send_msg (data.id, data.portno, data.msg);
      	  log_msg (data.id, data.msg);
      }
		
      mbuf_remove(io, io->len);      // Discard data from recv buffer
      break;
    default:
      break;
  }
}

bool
log_msg (char *id, char *msg)
{
	FILE *fp;
	time_t t = time (NULL);
	struct tm *tm = localtime (&t);
	char date[50];

	fp = fopen (id, "a");
	strcpy (date, asctime (tm));
	date[strlen (date)-1] = '\0';
	fprintf (fp, "{[%s] | [%s]}:\t%s\n", date, id, msg);
	fclose (fp);
	return true;
}


bool
send_msg (char *id, int portno, char *msg)
{
	int sockfd,  n;
	struct sockaddr_in address;
	struct hostent *server;

	sockfd = socket (AF_INET, SOCK_STREAM, 0);
	server = gethostbyname (id);
	address.sin_family = AF_INET;
	bcopy ((char *)server->h_addr_list[0], &address.sin_addr.s_addr, server->h_length);
	address.sin_port = htons (portno);
	connect (sockfd, (struct sockaddr *) &address, sizeof (address));
	write (sockfd, msg, strlen (msg));
	close (sockfd);
	return true;
}


int
main(int argc, char **argv) {
  struct mg_mgr mgr;

  if (strcmp (argv[2], "mong") == 0) {
  	  mg_mgr_init(&mgr, NULL);  // Initialize event manager object

  	  // Note that many connections can be added to a single event manager
  	  // Connections can be created at any point, e.g. in event handler function
  	  mg_bind(&mgr, argv[1], ev_handler);  // Create listening connection and add it to the event manager

  	  for (;;) {  // Start infinite event loop
    	mg_mgr_poll(&mgr, 1000);
  	  }

  	  mg_mgr_free(&mgr);
  } else {
  	  send_msg ("localhost", 1200, "ciao");
        /*break;*/
  }
  return 0;
}
