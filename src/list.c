#include "../lib/datastructs.h" // struct list
#include <string.h> // strdup
#include <stdlib.h> // malloc
#include "../lib/util.h" // get_date
#include <stdbool.h>
#include "../lib/socks_helper.h" // send_message_to_socket
#include "../include/uthash.h"
#include <pthread.h> // mutexes

static pthread_mutex_t *mut = NULL;

static struct peer *head = NULL;
// a pointer to the head of peer list

struct threadList {
	// not used
	// see main.c
	pthread_t *tid;
	struct threadList *next;
};

static struct peer *
new_peer (const char *id)
{
	// allocate a new peer list
	struct peer *new = calloc (1, sizeof (struct peer));
	if (new == NULL) {
		exit_error ("calloc");
	}
	strncpy (new->id, id, strlen (id));
	new->msg = NULL;
	return new;
}

bool
insert_peer (const char *onion)
{
	// first allocate a new peer node
	// then push it to the existing list
	// returns the head
	struct peer *new = new_peer (onion);
	if (mut == NULL) {
		mut = malloc (sizeof (pthread_mutex_t));
		pthread_mutex_init (mut, NULL);
	}
	if (pthread_mutex_lock (mut) != 0) {
		exit_error ("pthread mutex: ");
	}
	HASH_ADD_STR (head, id, new);
	pthread_mutex_unlock (mut);
// uthash modifies *p, so it can't be used again in other functions
	return true;
}

static struct peer *
get_peer (const char *id)
{
	struct peer *p;
	if (mut == NULL) {
		mut = malloc (sizeof (pthread_mutex_t));
		pthread_mutex_init (mut, NULL);
	}
	if (pthread_mutex_lock (mut) != 0) {
		exit_error ("pthread mutex: ");
	}
	HASH_FIND_STR (head, id, p); // returs null if doesn't exist
	pthread_mutex_unlock (mut);
	return p;
}

bool
peer_exist (const char *id)
{
	if (get_peer (id) == NULL) {
		return false;
	} else {
		return true;
	}
}

static struct message *
new_message (const char *content)
{
	// first allocate a new message node
	// then insert content and date
	struct message *new = malloc (sizeof (struct message));

	if (new == NULL) {
		exit_error ("malloc");
	}
	new->next = NULL;
	new->prev = NULL;
	new->date = get_short_date ();
	new->content = strdup (content);
	return new;
}

static struct message *
get_tail (struct message *h)
{
	// get to most recent message, the tail
	while (h->next != NULL) {
		h = h->next;
	}
	return h;
}

bool
insert_new_message  (const char *peerId, const char *content)
{
	// insert a new message to a message list
	// most recent at tail 
	// returns oldest message as head
	//
	// does not check that peer exist
	struct peer *p = get_peer (peerId);
	struct message *new = new_message (content);
	if(p->msg == NULL){
		p->msg = new;
	} else {
		struct message *tmp = get_tail (p->msg);
		tmp->next = new;
		new->prev = tmp;
	}
	return true;
}

static struct message *
delete_message (struct message *msg)
{
	// frees the message and deletes its content
	// delete all messages
	struct message *tmp;
	free(msg->content);
	free(msg->date);
	tmp = msg;
	msg = msg->next;
	free(tmp);
	return msg;
}

static void
delete_peer(struct peer *currentPeer)
{
	// frees the peer given and deletes its id
	// note: here we suppose that the msg list
	// of the peer has already been freed (see delete_message)
	if (mut == NULL) {
		mut = malloc (sizeof (pthread_mutex_t));
		pthread_mutex_init (mut, NULL);
	}
	if (pthread_mutex_lock (mut) != 0) {
		exit_error ("pthread mutex: ");
	}
	HASH_DEL (head, currentPeer);
	if (currentPeer->msg != NULL) {
		free (currentPeer->msg);
	}
	pthread_mutex_unlock (mut);
}

char *
get_unread_message(const char *peerId)
{
	// for the given peer
	// check if there are messages that need to be read
	// if there are any, return the oldest one
	// otherwise, NULL is returned
	struct peer *currentPeer = get_peer(peerId);
	if(currentPeer == NULL){
		return NULL;
	}
	struct message *msg = currentPeer->msg;
	if(msg == NULL){
		return NULL;
	}
	/*int len = strlen(msg->content)+strlen(msg->date)+3;*/
	char *m = strdup(msg->content);
	currentPeer->msg = delete_message (currentPeer->msg);
	if (currentPeer->msg == NULL) {
		// if we read every message of the peer, remove peer from hash table
		delete_peer (currentPeer);
	}
	
	return m;
}

struct peer *
get_list_head()
{
	return head;
}

char *
get_peer_list ()
{
	// get a list of the peers that send the server a message the client still didn't read
	// this should be parsed as a json after
	struct peer *ptr, *tmp; // used to iterate
	size_t size = 0; // size = sum of strlen of all peer id
	if (head == NULL) {
		return NULL;
	} else {
		if (mut == NULL) {
			mut = malloc (sizeof (pthread_mutex_t));
			pthread_mutex_init (mut, NULL);
		}
		if (pthread_mutex_lock (mut) != 0) {
			exit_error ("pthread mutex: ");
		}
		HASH_ITER (hh, head, ptr, tmp) {
			size += strlen (ptr->id) + 1; // +1 is for the comma
		}
		char *peerList = calloc (size + 1, sizeof (char));
		HASH_ITER (hh, head, ptr, tmp) {
			// iterate again and concatenate the char*
			strncat (peerList, ptr->id, strlen (ptr->id));
			strncat (peerList, ",", sizeof (char));
		}
		peerList[strlen (peerList) - 1] = '\0';
		pthread_mutex_unlock (mut);
		return peerList; // already in heap
	}
}

void
clear_datastructs ()
{
	struct peer *ptr, *tmp; // used to iterate
	HASH_ITER (hh, head, ptr, tmp) {
		delete_peer (ptr);
	}
}

void
destroy_mut()
{
	// avoid closing without freeing the mutex allocated memory
	if(mut != NULL){
		free(mut);
	}
}
