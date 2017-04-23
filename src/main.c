#include <stdio.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h> //umask
#include <assert.h> // assert
#include "include/mem.h" // CALLOC and MALLOC
#include "lib/datastructs.h"
#include "lib/socks_helper.h"
#include "lib/util.h"
#include "lib/actions.h" // event_routine functions
#include "include/libdill.h"
#include "include/fd.h" // libdill fd
#include "lib/torchatproto.h"

#define MAXEVENTS 64
#define MAXCONN 16 
extern struct data_wrapper *convert_string_to_datastruct (const char *jsonCh); // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper *data); // from json.cpp
extern void log_info (char *json); // from logger.cpp
extern void log_err (char *json); // from logger.cpp
void log_init (char *name, char *verbosity); // from logger.cpp
void log_clear_datastructs (); // from logger.cpp

static bool exitFlag = false; // this flag is set to true when the program should exit

char *HOSTNAME = NULL; // will be read from torrc

static void
start_daemon()
{
    /* this function daemonizes the main core of the chat
	 * while in daemon mode, only logging can be used
	 * to monitor the server activity
	 */
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
    dir = CALLOC (200, sizeof(char));
	getcwd(dir, 200);	
	chdir(dir);
	free(dir);

	/*close all open file descriptors*/
	/*this is needed in order to stop in/out to/from stdin, stdout, stderr*/
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
    return STRDUP (buf);
}

coroutine void
event_routine (const int rawsock) 
{
	int sock = torchatproto_attach (rawsock); // TODO find a way to epoll torprotofd
	// manage server RECV events
	struct data_wrapper *data = NULL;
	char *json = NULL;

	// the client opens a new socket every message, and closes the previous
	// this means that the server must perform only one recv then close thne coroutine
	/*int64_t deadline = now() + TOR_TIMEOUT; // two minutes deadline*/
	int64_t deadline = now () + 3000;
	/*int rc = parse_connection(sock, &data, &json, deadline);*/
	int rc = parse_connection(sock, &data, &json, deadline);
		
	if (rc > 0) { 
    	switch (data->cmd) {
    		case EXIT :
        		exitFlag = true;
        		log_info (json);
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
        		relay_msg (sock, data, deadline);
				free_data_wrapper (data);
				// data wrapper is free'd in routine 
        		break;
    		case UPDATE:
        		client_update (data, sock, deadline);
				free_data_wrapper (data);
        		break;
    		case GET_PEERS :
        		send_peer_list_to_client (data, sock, deadline);
				free_data_wrapper (data);
        		break;
			case HOST :
				// the client required the hostname of the server
				// send as a formatted json
				send_hostname_to_client(data, sock, HOSTNAME, deadline);
				free_data_wrapper (data);
				break;
    		default:
				free_data_wrapper (data);
        		break;
    	}
	} else if (rc == 0) {
		// invalid message, close connection
		close (rawsock);
	}
	
	int oldrawsock = torchatproto_detach (sock);
	assert (rawsock == oldrawsock);

	FREE (json);
	// data should be freed inside the jump table because it can be used in threads
    return;
}

void event_loop (const int listenSock)
{
	// this is the event loop,
	// uses epoll
	// closes fds at exit
	int nConn = 0;
	bool trackFd[MAXCONN] = {false}; // used to track all the accepted connection and close them at the end
  	int efd;
  	struct epoll_event event = {0};
  	struct epoll_event *events;
  	efd = epoll_create1(0);
  	assert (efd != -1);

  	event.data.fd = listenSock;
  	event.events = EPOLLIN | EPOLLET;
  	int rc = epoll_ctl(efd, EPOLL_CTL_ADD, listenSock, &event);
  	assert (rc != -1);

  	/* Buffer where events are returned */
  	events = CALLOC(MAXEVENTS, sizeof event);

  	/* The event loop */
	while (!exitFlag) {  // start poll loop
    	int n, i;
    	n = epoll_wait(efd, events, MAXEVENTS, -1);
    	yield ();
    	for (i = 0; i < n; i++) {
      		if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))) {
        		/* An error has occured on this fd, or the socket is not
           		   ready for reading (why were we notified then?) */
                /*fprintf(stderr, "epoll error\n%s\n", strerror (errno));*/
				close(events[i].data.fd);
				trackFd[events[i].data.fd] = false;
				nConn--;
        		continue;
      		}

      		else if (listenSock == events[i].data.fd) {
        		/* We have a notification on the listening socket, which
           		   means one or more incoming connections. */
        		while (true) {
        			// accept all connections
          			struct sockaddr in_addr;
          			socklen_t in_len;
          			int infd;
          			char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

          			in_len = sizeof in_addr;
          			infd = accept(listenSock, &in_addr, &in_len);
					trackFd[infd] = true; // keep track of accepted connections
					nConn++;
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
          			rc = torchatproto_fd_unblock (infd);
					/*int	sock = torchatproto_attach (infd); // from now on communicate using torchat protocol*/
          			assert (rc != -1);

          			event.data.fd = infd;
          			event.events = EPOLLIN | EPOLLET;
          			rc = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
          			assert (rc != -1);
        		}
        		continue;
      		}

      		else {
        		/* We have data on the fd waiting to be read. 
           		   we are running in edge-triggered mode
           		   and won't get a notification again for the same data. */
				int cr = go(event_routine(events[i].data.fd));
          		/* Closing the descriptor will make epoll remove it
             	   from the set of descriptors which are monitored. */
                /*close(events[i].data.fd);*/
            }
        }
    }

	FREE (events);
	close (efd);
	nConn--;
	while (nConn >= 0) { shutdown (trackFd[nConn], SHUT_RDWR); close (trackFd[nConn--]); }

	return;
}

void 
exit_from_signal (int sig) 
{
	// issue a shutdown after receiving CTRL C
	fprintf (stdout, "Interrupt: exiting cleanly.\n");
	exitFlag = true;
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf (stdout, "USAGE...\n");
		exit (EXIT_FAILURE);
	}
	int port;
	if(strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--daemon") == 0) {
        start_daemon();
    	port = atoi (argv[2]);
    } else {
    	port = atoi (argv[1]);
	}

#ifndef NDEBUG
    signal (SIGSEGV, dumpstack);
    /*signal (SIGABRT, dumpstack);*/
    log_init ("debug.log", "DEBUG");
#endif
	signal (SIGINT, exit_from_signal);
    log_init ("file.log", "INFO");
    log_init ("error.log", "ERROR");

	// initialization of datastructs
    HOSTNAME = read_tor_hostname ();
	// initialize struct needed to connect with SOCKS5 TOR
	initialize_proxy_connection ("localhost", 9250);

	int listenSock = bind_and_listen (8000, 10);

	// core of the program, go into event loop
	event_loop (listenSock);

	shutdown(listenSock, SHUT_RDWR);
	close (listenSock);
	// closed the listening socket
    clear_datastructs (); // free hash table entries
    log_clear_datastructs (); // free static vars in logger.cpp
    FREE (HOSTNAME);
    // terminate SOCKS5 structs
    destroy_proxy_connection ();

    exit (EXIT_SUCCESS);
}
