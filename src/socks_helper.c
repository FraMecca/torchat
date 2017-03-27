#include <string.h> // strdup
#include <strings.h> // bcopy
#include <stdbool.h> // bool
#include <errno.h> // perror
#include "include/libdill.h" // yield
#include <stdio.h> // perror
#include "lib/datastructs.h" // for message to socket
#include "lib/util.h"
#include "include/mem.h"
#include <unistd.h> // close
#include "include/proxysocket.h"

static char torError;
static proxysocketconfig proxy = NULL;

// wrapper for sockets
// ****************** //

bool 
set_socket_timeout (const int sockfd)
{
	// this function sets the socket timeout
	// 120s
	    struct timeval timeout;      
	    timeout.tv_sec = 120;
	    timeout.tv_usec = 0;

	    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
	        perror("setsockopt failed\n");
	        return false;
	    }

	    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
	        perror("setsockopt failed\n");
	        return false;
	    }
	    return true;
}

ssize_t 
crlf_send (int sock, const char *buf, size_t len)
{

	char tmpB[len + 2];

	strncpy (tmpB, buf, len);
	if (buf[len - 1] == '}') { // implying everything is a json
		// buf is not crlf terminated
		tmpB[len] = '\r'; tmpB[len + 1] = '\n'; tmpB[len + 2] = '\0';
		len += 2;
	}
	return send (sock, tmpB, len, 0);
}

static bool
line_is_terminated (char *b, size_t len)
{
	// just check if \r\n at b[len]	
	if (b[len - 1] == '\n' && b[len - 2] == '\r') {
		return true;
	} else {
		return false;
	}
}

ssize_t
crlf_recv (const int sock, char *retBuf, const size_t maxSz)
{
	// it is the same as (3) recv
	// but it receives a buffer that is \r\n terminated only
	// it reads until size sz or \r\n and return -1 if the buffer read is not valid (not correctly terminated)
	// IT IS BLOCKING
	// requires retBuf to be allocated already 
	retBuf[0] = '\0'; // strncat now starts from the beginning of retBuf
	char buf[maxSz];
	ssize_t sz = maxSz, finalSize = 0, rsz = 0;
	
	while ((rsz = recv (sock, buf, sz, 0)) > 0) {
		strncat (retBuf, buf, rsz); // store received chars in buf because tmp will be overwritten
		// should use a more efficent datastruct
		finalSize += rsz;
		sz -= rsz;
		if (line_is_terminated (buf, finalSize)) {
			return finalSize - 2;
		}
		yield();
	}
	return rsz; // recv failed (-1) or closed connection (0)
}

int
bind_and_listen (const int portno)
{
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
    if (listen(sockfd, 8) != -1) {
    	return sockfd;
    } else {
    	// error on listen, check errno
    	return -1;
    }
}

// ************* //

void
initialize_proxy_connection (const char *host, const int port)
{
	// call destroy for freeing memory
	// initialize proxysocket lib handlers
	proxysocket_initialize();
	proxy = proxysocketconfig_create_direct();
	/*int verbose = PROXYSOCKET_LOG_DEBUG; // can be removed*/
	/*proxysocketconfig_set_logging(proxy, logger, (int*)&verbose);*/
	proxysocketconfig_use_proxy_dns(proxy, 1); // use TOR for name resolution
	proxysocketconfig_add_proxy(proxy, PROXYSOCKET_TYPE_SOCKS5, host, port, NULL, NULL);
}

void
terminate_connection_with_domain (const int sock)
{
	// disconnect from a domain
    proxysocket_disconnect(proxy, sock);
}

void 
destroy_proxy_connection ()
{
  	proxysocketconfig_free(proxy);
}

static void
set_tor_error(const char e)
{
	torError = e;
}

char *
get_tor_error ()
{
	 // in case of error the message returned to the client has in the msg field
	 // an explanation of the error, see http://www.ietf.org/rfc/rfc1928.txt
	switch (torError) {
		case 1 :
			return STRDUP ("general SOCKS server failure");
		case 2 :
			return STRDUP ("connection not allowed by ruleset");
		case 3 :
			return STRDUP ("Network unreachable");
		case 4 :
			return STRDUP ("Host unreachable");
		case 5 :
			return STRDUP ("Connection refused");
		case 6 : 
			return STRDUP ("TTL expired");
		case 7 :
			return STRDUP ("Command not supported");
		case 8 :
			return STRDUP ("Address type not supported");
		case 9 :
			// not in RFC,
			// used when handshake fails, so probably TOR has not been started
			return STRDUP ("Could not send message. Is TOR running?");
		case 10 :
			return STRDUP("Failed handshake with TOR.");
		default :
			return STRDUP ("TOR couldn't send the message"); // shouldn't go here
	}
}

int
open_socket_to_domain(const char *domain, const int portno)
{
	// connect to an .onion domain
	// return socket
	//connect
  	char* errmsg;
	return proxysocket_connect(proxy, domain, portno, &errmsg);

}

ssize_t
send_over_tor (const char *domain, const int port, char *buf)
{
	// wraps functions above, 
	// assumes that buf is null terminated
	int sock = open_socket_to_domain (domain, port);
	if (sock != -1) {
		return crlf_send (sock, buf, strlen (buf));
	}
	return -1; // error opening socket
}

