#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
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
#include <errno.h>



bool
send_over_tor (const char *domain, const int portno, const char *buf, const int torPort)
{ 
	// buf is the json parsed char to be sent over a socket

    int sock;
    struct sockaddr_in socketAddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
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
        return false ; // Error, handshake failed ?
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
    printf ("sock_helper:70 %s\n", req2);

    char Resp2[10];
    recv(sock, Resp2, 10, 0);

    // Here you can normally use send and recv
    // Testing With a HTTP GET Request
    send(sock, buf, strlen (buf), 0);

    return true;
}
