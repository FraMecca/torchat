#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "include/proxysocket/src/proxysocket.h"
#include "lib/tc_util.h"

static proxysocketconfig proxy = NULL;

/****************************************************************/
/*                 SOCKET RELATED FUNCTIONS                     */
/****************************************************************/
int
fd_unblock (int fd)
{
	// set socket to non blocking
	// return status of operation
    int flags;

    /* If they have O_NONBLOCK, use the Posix way to do it */
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int
fd_connect (char *address, int port)
{
    int sock;
    struct sockaddr_in server;
    sock = socket(AF_INET , SOCK_STREAM , 0);
    assert (sock != -1);
    server.sin_addr.s_addr = inet_addr(addr);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
 
    connect(sock , (struct sockaddr *)&server , sizeof(server));
    return sock;
}

int
bind_and_listen (const int portno, int n)
{
	// this functions simply wraps
	// bind
	// listen
	//
	// n is the listen parameter
    struct sockaddr_in servAddr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { 
        exit_error("ERROR opening socket");
    }
    bzero((char *) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY; // TODO: bind to a selected interface
    servAddr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        exit_error("ERROR on binding");
    }
    int rc = fd_unblock (sockfd);
    assert (rc >= 0);
    if (listen(sockfd, n) != -1) {
    	return sockfd;
    } else {
    	// error on listen, check errno
    	return -1;
    }
}

/****************************************************************/
/*                 TOR RELATED FUNCTIONS                        */
/****************************************************************/
/*void*/
/*disconnect (const int sock)*/
/*{*/
	/*// disconnect from a domain*/
	/*proxysocket_disconnect(proxy, sock);*/
	/*close (sock);*/
/*}*/

/*int*/
/*open_socket_to_domain(const char *domain, const int portno)*/
/*{*/
	/*// connect to an .onion domain*/
	/*// return socket*/
	/*// connect*/
	/*// BLOCKING*/
	/*char* errmsg;*/
	/*return proxysocket_connect(proxy, domain, portno, &errmsg);*/
/*}*/

void
connect_to_tor (const char *host, const int port)
{
	// initialize proxysocket lib handlers
	// used to open a connection to the tor daemon
	// in order to have connections through socks5
	proxysocket_initialize();
	proxy = proxysocketconfig_create_direct();
	/*int verbose = PROXYSOCKET_LOG_DEBUG; // can be removed*/
	/*proxysocketconfig_set_logging(proxy, logger, (int*)&verbose);*/
	proxysocketconfig_use_proxy_dns(proxy, 1); // use TOR for name resolution
	proxysocketconfig_add_proxy(proxy, PROXYSOCKET_TYPE_SOCKS5, host, port, NULL, NULL);
}

void
destroy_proxy_connection ()
{
	proxysocketconfig_free(proxy);
}
