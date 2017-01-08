#include <arpa/inet.h> // inet
#include <string.h> // strdup
#include <stdlib.h> // calloc
#include <netinet/in.h> // socket types
#include <strings.h> // bcopy
#include <stdbool.h> // bool
#include <time.h> // timestructs
#include <errno.h> // perror
#include <stdio.h> // perror
#include "../lib/datastructs.h" // for message to socket
#include <unistd.h> // close


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

bool
send_message_to_socket(struct message *msgStruct, char *peerId)
{
	// the daemon uses a socket
	// to send unread messages to the client
	int sock;
	struct sockaddr_in socketAddr;
	char *msgBuf;
	int msgSize;
	
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = 42000; // yeah that was random
	socketAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(sock, (struct sockaddr*)&socketAddr, sizeof(struct sockaddr_in)) < 0){
		perror("socket connect");
		return false;
	}

	// message of the form: [ID] time: message
	// real programmers don't care about formatting
	msgSize = strlen(peerId)+strlen(msgStruct->content)+strlen(msgStruct->date)+7;
	// everything is better if the buffer is zeroed
	msgBuf = calloc(msgSize, sizeof(char));

	// ops! C strings did it again
	msgBuf[0] = '[';
	strncat(msgBuf, peerId, strlen (peerId));
	strncat(msgBuf, "]", 1);
	strncat(msgBuf, msgStruct->date, strlen (msgStruct->date));
	strncat(msgBuf, ":", 1);
	strncat(msgBuf, msgStruct->content, strlen (msgStruct->content));

	// sending the message to the socket
	if(send(sock, msgBuf, strlen(msgBuf), 0) < 0){
		perror("send");
		return false;
	}
	
	free(msgBuf);
	free(peerId);
	return true;
}

char
send_over_tor (const char *domain, const int portno, const char *buf, const int torPort)
{ 
	// buf is the json parsed char to be sent over a socket
    int sock;
    struct sockaddr_in socketAddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (!set_socket_timeout (sock)) {
    	perror ("setsockopt");
    	exit (1);
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
    send(sock, handShake, 3, 0);

    char resp1[2];
    recv(sock, resp1, 2, 0);
    if(resp1[1] != 0x00)    {
        return resp1[1]; // Error, handshake failed ?
    }

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

    char resp2[10];
    recv(sock, resp2, 10, 0);
    // if resp2 is ok, tor opened a socket to domain (.onion)
	if(resp2[1] != 0){
		return resp2[1];
	}

    // Here you can normally use send and recv
    // buf is the message I want to send to peer
    if (send(sock, buf, strlen (buf), 0) < 0) {
    	return -1;
    	// shouldn't
    }

	close (sock);
    return 0;
}
