#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "lib/tc_handle.h"
#include "lib/tc_mem.h"
#include "lib/tc_sockets.h"
#include "lib/tc_messages.h"
#include "lib/tc_util.h"
#include "lib/tc_event.h"
 
#define TC_TYPE_MESSAGE 'm'
#define TC_TYPE_FILE	'f'

static bool exitFlag = false;

void
stop_loop (int signum)
{
	signal (SIGALRM, exit_on_stall);
	alarm (10);
	exitFlag = true;
}

static int
tc_attach (int fd)
{
	// determine type of message received
	// and attach to the correct handler
	//
	// read the first message (the opening one)
	// in which the type is specified
	// we use tc_message_attach
	// because we are sure that the first json is a message
	unsigned char buf[MSIZEMAX] = {0}; 
	int tfd = tc_message_attach(fd);
	int rc = tc_mrecv(fd, buf);
	// TODO determine if the fd must be closed here
	/*if (rc == 0) tc_mclose(fd);*/

	// check type here
	// TODO implement type checking, now is just a char
	// to determine type, read and parse the json
	// one of the tokens is "open": "type"
	char type = TC_TYPE_MESSAGE;
	int nfd;

	if (type == TC_TYPE_MESSAGE){
		nfd = tc_message_attach(tfd);
	} else if (type == TC_TYPE_FILE) {
		// TODO implement file handle functions
		/*nfd = tc_file_attach(tfd);*/
	} else {
		// TODO notify error properly
		fprintf(stderr, "Invalid file type: %c\n", type);
		return -1;
	}
	return nfd;
}

static int
tc_dispatch (int fd)
{
	// first check if fd present in one of our handlers
	// if not, determine type of stream and attach vtfs
	// then read content
	// and start routine based on content of jmu

	// TODO: make a generic attach function and further abstraction to initialize or check vfs

	struct vfsTable_t *t = tc_query (fd);	
	int nfd;
	if (t == NULL) { 
		// should determine type
		/*[>tc_determine_type (<]*/
		/*nfd = tc_message_attach (fd);*/
		/*[>[>assert (nfd != -1);<]<]*/
		/*fprintf (stderr, "TODO: generalize attch fn\n");*/
		nfd = tc_attach(fd);

	} else { nfd = fd; }

	// does not work if no tc_message_attach,
	// correct in the morning
	unsigned char buf[MSIZEMAX] = {0};
	int rc = tc_mrecv (nfd, buf);
	if (rc == 0) tc_mclose (nfd);
	else { fprintf (stdout, "%s\n", buf); tc_msend (nfd, (unsigned char *)"wuuuuuuut", 9); }

	return 1;
}

void event_loop (const int listenSock)
{
	// this is the event loop,
	// uses epoll
	// closes fds at exit
  	int efd;
  	struct epoll_event event = {0};
  	autofree struct epoll_event *events;
  	efd = epoll_create1(0);
  	assert (efd != -1);

  	event.data.fd = listenSock;
  	event.events = EPOLLFLAGS;
  	int rc = epoll_ctl(efd, EPOLL_CTL_ADD, listenSock, &event);
  	assert (rc != -1);

  	/* Buffer where events are returned */
  	events = calloc (MAXEVENTS, sizeof event);

  	/* The event loop */
	while (!exitFlag) {  // start poll loop
    	int n, i;
		// timeout to 120 sec
    	n = epoll_wait(efd, events, MAXEVENTS, 120000);
    	for (i = 0; i < n; i++) {
      		if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (events[i].events & EPOLLRDHUP) || (!(events[i].events & EPOLLIN))) {
        		/* An error has occured on this fd, or the socket is not
           		   ready for reading (why were we notified then?) */
                /*fprintf(stderr, "epoll error\n%s\n", strerror (errno));*/
				close(events[i].data.fd);
        		continue;
      		} else if (listenSock == events[i].data.fd) {
        		/* We have a notification on the listening socket, which
           		   means one or more incoming connections. */
        		while (true) {
        			// accept all connections
          			struct sockaddr in_addr;
          			socklen_t in_len;
          			int infd;

          			in_len = sizeof in_addr;
          			infd = accept(listenSock, &in_addr, &in_len);
          			if (infd == -1) {
              			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
              	  			/* We have processed all incoming
                 	 		   connections. */
              	  			break;
              			} else {
              	  			perror("accept");
              	  			break;
              			}
          			}
          			/* Make the incoming socket non-blocking and add it to the
             		   list of fds to monitor. */
          			rc = fd_unblock (infd);
          			assert (rc != -1);

          			event.data.fd = infd;
          			event.events = EPOLLFLAGS;
          			rc = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
          			assert (rc != -1);
        		}
        		continue;
      		} else {
        		/* We have data on the fd waiting to be read or written. 
           		   we are running in edge-triggered mode
           		   and won't get a notification again for the same data. */
           		int infd = events[i].data.fd;
           		// now let torchat proto handle the fd
      			int cr = tc_dispatch (infd); 
      			assert (cr != -1);
            }
        }
    }

	/*FREE (events);*/
	close (efd);
	return;
}
