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
#include "lib/tc_client.h"
#include "lib/tc_util.h"
#include "lib/tc_event.h"
 
static bool exitFlag = false;

void
stop_loop (int signum)
{
	signal (SIGALRM, exit_on_stall);
	alarm (10);
	exitFlag = true;
}

int
tc_read_open_jmu (int fd, char *buf)
{
	// read the first jmu that is sent
	// when another node wants to start a communication
	// read the jmu 
	// in order to understand the type
	uint16_t size;
	int rc = read (fd, &size, sizeof (uint16_t));
	if (rc <= 0) return rc;
	// now convert size that is noted at begin of the message 
	// and use it to read correctly
	if (size > MSIZEMAX) { errno = EMSGSIZE; return -1; } 
	rc = read (fd, buf, size);
	assert (rc == -1 || size == rc);
	return rc;
}

static enum type
get_type (char *buf)
{
	// TODO of course, improve
	customfree (destroy_json) JSON *j = json_parse (buf);
	if (j == NULL) return -1;
	if (json_get (j, "auth") != NULL && json_get (j, "to") != NULL) {
		return CLIENT_T;
	} else {
		return MESSAGE_T;
	}
}
static int
tc_attach (int fd)
{
	// determine type of message received
	// and attach to the correct handler
	//
	// read the first message (the opening one)
	// in which the type is specified
	char buf[MSIZEMAX] = {0}; 
	/*int tfd = tc_message_attach(fd);*/
	int rc = tc_read_open_jmu (fd, buf);
	// TODO determine if the fd must be closed here
	/*if (rc == 0) tc_mclose(fd);*/

	// check type here
	// TODO implement type checking, now is just a char
	// to determine type, read and parse the json
	// one of the tokens is "open": "type"
	enum type type = get_type (buf);
	int nfd;

	if (type == MESSAGE_T){
		nfd = tc_message_attach(fd); // also sends an ack to sender
	} else if (type == FILE_T) {
		// TODO implement file handle functions
		/*nfd = tc_file_attach(fd);*/
	} else if (type == CLIENT_T) {
		tc_client_attach (fd);
		/*nfd = tc_file_attach(fd);*/
	} else {
		// TODO notify error properly
		fprintf(stderr, "Invalid file type: %c\n", type);
		return -1;
	}
	return nfd;
}

static int
tc_send_ack (struct vfsTable_t *t)
{
	char buf[] = "{\"version\":\"1\", \"ack\":true, \"from\":\"demo\"}";
	customfree (destroy_json) JSON *j = json_parse (buf);
	assert (j != NULL);
	int rc = t->send (t->fd, j);
	return rc;
}

static int
tc_dispatch (int fd)
{
	// first check if fd present in one of our handlers
	// if not, determine type of stream and attach vtfs
	// then read content
	// and start routine based on content of jmu

	struct vfsTable_t *t = tc_query (fd);	
	int nfd;
	if (t == NULL) { 
		// should determine type
		nfd = tc_attach(fd);
		t = tc_query (fd);	
		if (nfd == -1) return -1;
		// now send ack
		tc_send_ack (t);
	} else {
		// this is NOT an opening jmu, handle it
		int rc = t->recv (t->fd);
		assert (rc != -1); // TODO check errno
		if (rc == 0) t->close (t->fd);
	}

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
