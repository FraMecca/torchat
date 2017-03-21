#include <arpa/inet.h> // inet
#include <string.h> // strdup
#include <stdlib.h> // calloc
#include <netinet/in.h> // socket types
#include <strings.h> // bcopy
#include <stdbool.h> // bool
#include <time.h> // timestructs
#include <errno.h> // perror
#include <stdio.h> // perror
#include "lib/datastructs.h" // for message to socket
#include "lib/util.h"
#include "include/mem.h"
#include <unistd.h> // close

static char torError;

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



static bool 
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

int
handshake_with_tor (const int torPort)
{
	// do an handshake with the TOR daemon
    int sock;
    struct sockaddr_in socketAddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (!set_socket_timeout (sock)) {
    	exit_error("setsockopt");
    }
    socketAddr.sin_family       = AF_INET;
    socketAddr.sin_port         = htons(torPort);
    socketAddr.sin_addr.s_addr  = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr*)&socketAddr, sizeof(struct sockaddr_in));

    char handShake[3] =
    {
        0x05, // SOCKS 5
        0x01, // One Authentication Method
        0x00  // No AUthentication
    };
    if(send(sock, handShake, 3, 0) >= 0){
		return sock;
	} else {
		set_tor_error(10);
		return -1; // check errno
	}
}

int
open_socket_to_domain(int sock, const char *domain, const int portno)
{
	// connect to an .onion domain
	// the second part of the handshake
    char  domainLen = (char)strlen(domain);
    short port = htons(portno);

    char TmpReq[4] = {
          0x05, // SOCKS5
          0x01, // CONNECT
          0x00, // RESERVED
          0x03, // domain
        };

    char* req2 = malloc ((4+1+domainLen + 2) *sizeof (char));

    memcpy(req2, TmpReq, 4);                // 5, 1, 0, 3
    memcpy(req2 + 4, &domainLen, 1);        // Domain Length
    memcpy(req2 + 5, domain, domainLen);    // Domain
    memcpy(req2 + 5 + domainLen, &port, 2); // Port

    send(sock, (char*)req2, 4 + 1 + domainLen + 2, 0);
    free (req2);

    char resp2[10] = {0};
    recv(sock, resp2, 10, 0);
    // if resp2 is ok, tor opened a socket to domain (.onion)
	if(resp2[1] != 0){
		// set a global to the error value
		set_tor_error(resp2[1]);
		return -1; 
	}
	return 0;
}

static int 
send_crlf (int sock, const char *buf, size_t len, int f)
{

	char tmpB[len + 2];
	strncpy (tmpB, buf, len);
	if (buf[len - 1] == '}') { // implying everything is a json
		// buf is not crlf terminated
		tmpB[len] = '\r'; tmpB[len + 1] = '\n'; tmpB[len + 2] = '\0';
		len += 2;
	}
	return send (sock, tmpB, len, f);
}

int
send_over_tor (const int torSock, const char *domain, const int portno, const char *buf)
{
	// buf is the json parsed char to be sent over a socket
	// buf is the message I want to send to peer
	// torSock is obtained by the handshake in main.c, event_routine
	// here we connect to the domain
	if (open_socket_to_domain(torSock, domain, portno) != 0){
		return -1;	
	}
	// here we send
	if (send_crlf(torSock, buf, strlen (buf), 0) < 0) {
		return -1;
	}
	return 0;
}
