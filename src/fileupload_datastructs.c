#include "lib/file_upload.h"
#include "lib/datastructs.h"
#include "lib/actions.h"
#include "lib/socks_helper.h"
#include "include/mem.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "include/uthash.h"

static pthread_mutex_t *mut = NULL;
static struct openFiles *head = NULL;
// a pointer to the head of files <-> sockets hash table
//
struct openFiles {
	char *host;
	int sockfd;
	int port;
	UT_hash_handle hh; // hash table handler
};

static struct openFiles *
new_file_and_socket (const char *host, const int port)
{
	// allocate a new peer list
	struct openFiles *newF = CALLOC (1, sizeof (struct peer));
	if (newF == NULL) {
		exit_error ("CALLOC");
	}
	/*newF->sockfd = -1;*/
	newF->sockfd =	handshake_with_tor (host, port, 9250);
	newF->host = STRDUP (host);
	newF->port = port;
	return newF;
}

static bool
insert_new_file_and_socket (const char *onion, int port)
{
	// first allocate a new peer node
	// then push it to the existing list
	// returns the head
	struct openFiles *newF = new_file_and_socket (onion, port);
	if (mut == NULL) {
		mut = MALLOC (sizeof (pthread_mutex_t));
		pthread_mutex_init (mut, NULL);
	}
	if (pthread_mutex_lock (mut) != 0) {
		exit_error ("pthread mutex: ");
	}
	HASH_ADD_STR (head, host, newF);
	pthread_mutex_unlock (mut);
// uthash modifies *p, so it can't be used again in other functions
	return true;
}

int
get_socket_related_to_host (const char *host, int port)
{
	 // TODO: change everything with filestruct, not host port
	 // also, port is hardcoded to 80 because the sender
	struct openFiles *p;
	port = 80;
	if (mut == NULL) {
		mut = MALLOC (sizeof (pthread_mutex_t));
		pthread_mutex_init (mut, NULL);
	}
	if (pthread_mutex_lock (mut) != 0) {
		exit_error ("pthread mutex: ");
	}
	HASH_FIND_STR (head, host, p); // returs null if doesn't exist
	pthread_mutex_unlock (mut);

	if (!p) {
		// this is a new peer, allocate a socket for it
		insert_new_file_and_socket (host, 80);
		if (pthread_mutex_lock (mut) != 0) {
			exit_error ("pthread mutex: ");
		}
		HASH_FIND_STR (head, host, p); // now peer was added to hash table
		pthread_mutex_unlock (mut);
	}
	return p->sockfd;
}
