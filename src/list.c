#include "../lib/datastructs.h" // struct list
#include <string.h> // strdup
#include <stdlib.h> // malloc
#include "../lib/util.h" // get_date
#include <stdbool.h>
#include "../lib/socks_helper.h" // send_message_to_socket

static struct peer *head = NULL;
// a pointer to the head of peer list

static struct peer *
new_peer (const char *id)
{
	// allocate a new peer list
	struct peer *new = malloc (sizeof (struct peer));
	if (new == NULL) {
		exit_error ("malloc");
	}
	strncpy (new->id, id, strlen (id));
	new->next = NULL;
	new->prev = NULL;
	new->msg = NULL;
	return new;
}

bool
insert_peer (const char *id)
{
	// first allocate a new peer node
	// then push it to the existing list
	// returns the head
	struct peer *new = new_peer (id);
	if (head == NULL) {
		head = new;
		return true;
	}
	new->next = head;
	head->prev = new;
	head = new;
	return true;
}

static struct peer *
get_peer (struct peer *current, const char *id)
{
	// get the node of a peer 
	// by iterating on the list
	// returns null if the id is not found
	//
	// needs head because recursion
	if (current == NULL) {
		return NULL;
		// id not found
	} else if (strcmp (current->id, id) == 0) {
		return current;
	} else {
		return get_peer (current->next, id);
	}
}

bool
peer_exist (char *id)
{
	if (get_peer (head, id) == NULL) {
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
	new->date = get_date ();
	new->content = strdup (content);
	return new;
}

bool
insert_new_message  (const char *peerId, const char *content)
{
	// insert a new message to a message list
	// most recent at top
	// returns new message as head
	//
	// does not check that peer exist
	struct peer *p = get_peer (head, peerId);
	struct message *new = new_message (content);
	if(p->msg == NULL){
		p->msg = new;
	} else {
		p->msg->prev = new;
		new->next = p->msg;
		p->msg = new;
	}
	return true;
}

struct message *
delete_message(struct message *msg)
{
	// frees the message and deletes its content
	// returns the next message
	struct message *m = msg->next;
	free(msg->content);
	free(msg->date);
	free(msg);
	return(m);
}

struct peer *
delete_peer(struct peer *currentPeer)
{
	// frees the peer given and deletes its id
	// note: here we suppose that the msg list
	// of the peer has already been freed (see delete_message)
	struct peer *p = currentPeer->next;
	free(currentPeer->id);
	free(currentPeer);
	return p;
}

bool
get_unread_messages(struct peer *currentPeer)
{
	// for the given peer
	// check if there are messages that need to be read
	// if there are any, send them to a socket to the client
	struct message *curMsg;

	curMsg = currentPeer->msg;

	while(curMsg != NULL){
		// keep on trying until the message is sent
		// note: this MAY HANG the thread execution
		// and compromise realtime/message order
		// DA RIVEDERE
		while(!send_message_to_socket(curMsg, strdup(currentPeer->id)));
		curMsg = delete_message(curMsg);
	}
	return true;
}

struct peer *
get_list_head()
{
	return head;
}

bool
check_peers_for_messages(struct peer *currentPeer)
{
	// every non-null peer
	// should have pending messages
	// finds every peer, gets its messages, frees the peer
	if(currentPeer == NULL){
		return true;
	}
	get_unread_messages(currentPeer);
	currentPeer = delete_peer(currentPeer);
	return check_peers_for_messages(currentPeer);
}
