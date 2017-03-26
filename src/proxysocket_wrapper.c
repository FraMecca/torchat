#include "include/proxysocket.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static proxysocketconfig proxy = NULL;

static void 
logger (int level, const char* message, void* userdata)
{
  const char* lvl;
  if (level > *(int*)userdata)
    return;
  switch (level) {
    case PROXYSOCKET_LOG_ERROR   : lvl = "ERR"; break;
    case PROXYSOCKET_LOG_WARNING : lvl = "WRN"; break;
    case PROXYSOCKET_LOG_INFO    : lvl = "INF"; break;
    case PROXYSOCKET_LOG_DEBUG   : lvl = "DBG"; break;
    default                      : lvl = "???"; break;
  }
  fprintf(stdout, "%s: %s\n", lvl, message);
}

void
initialize_proxy_connection ()
{
	// call destroy for freeing memory
	// initialize proxysocket lib handlers
	proxysocket_initialize();
	proxy = proxysocketconfig_create_direct();
	/*int verbose = PROXYSOCKET_LOG_DEBUG; // can be removed*/
	/*proxysocketconfig_set_logging(proxy, logger, (int*)&verbose);*/
	proxysocketconfig_use_proxy_dns(proxy, 1); // use TOR for name resolution
	proxysocketconfig_add_proxy(proxy, PROXYSOCKET_TYPE_SOCKS5, "127.0.0.1", 9250, NULL, NULL);
}

int
handshake_with_tor (const char *domain, const int port)
{
	// return a socket that can be used normally
	// it connects to TOR

	SOCKET sock; // define int SOCKET
	//connect
  	char* errmsg;
	sock = proxysocket_connect(proxy, domain, port, &errmsg);
	return sock;
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

int main (int argc, char **argv)
{
	initialize_proxy_connection ();
	int sock = handshake_with_tor (argv[1], 80);
	char buf[] = "{\"\r\ncmd\":\"RECV\",\"id\":\"ld74fqvoxpu5yi73.onion\",\"msg\":\"You are welcome for thanking me for welcoming you for thanking me for welcoming you.\",\"portno\":80}\r\n";
	send (sock, buf, strlen (buf), 0);
	terminate_connection_with_domain (sock);
	printf ("hand\n");
	sock = handshake_with_tor (argv[2], 80);
	char buf2[] = "{\"cmd\":\"RECV\",\"id\":\"ld74fqvoxpu5yi73.onion\",\"msg\":\"You are welcome for thanking me for welcoming you for thanking me for welcoming you.\",\"portno\":80}\r\n";
	send (sock, buf2, strlen (buf), 0);
	terminate_connection_with_domain (sock);

	destroy_proxy_connection ();

	return 0;
}
