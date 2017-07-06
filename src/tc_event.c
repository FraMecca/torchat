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

#include "lib/tc_mem.h"
#include "lib/tc_sockets.h"
#include "lib/tc_util.h"


static bool exitFlag = false;

void
stop_loop (int signum)
{
	signal (SIGALRM, exit_on_stall);
	alarm (10);
	exitFlag = true;
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
      			int cr = fd_read (infd);
      			if (!cr) close (infd); //else fd_write (infd, buf, strlen(buf));
            }
        }
    }

	/*FREE (events);*/
	close (efd);
	return;
}
