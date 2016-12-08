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
peer_exist (const char *id)
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
	struct peer *p = get_peer (head, peerId);
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

static void
delete_messages (struct message *msg)
{
	// frees the message and deletes its content
	// delete all messages
	
	struct message *m = msg->next, *tmp;
	while (msg != NULL) {
		free(msg->content);
		free(msg->date);
		tmp = msg;
		msg = msg->next;
		free(tmp);
	}
}

struct peer *
delete_peer(struct peer *currentPeer)
{
	// frees the peer given and deletes its id
	// note: here we suppose that the msg list
	// of the peer has already been freed (see delete_message)
	struct peer *p = currentPeer->next;
	if (p != NULL) {
		p->prev = currentPeer->prev;
		currentPeer->prev->next = p;
	} else {
		currentPeer->prev = NULL;
	}
	free(currentPeer->id);
	free(currentPeer);
	return p;
}

bool
get_unread_messages(struct peer *currentPeer)
{
	// for the given peer
	// check if there are messages that need to be read
	// if there are any, send them with mong_recv 
	// Sends all messages in one time
	struct message *curMsg;

	curMsg = currentPeer->msg;

	while(curMsg != NULL){
		// keep on trying until the message is sent
		// note: this MAY HANG the thread execution
		// and compromise realtime/message order
		// DA RIVEDERE
		while(!send_message_to_socket(curMsg, strdup(currentPeer->id)));
		curMsg = curMsg->next;
	}
	delete_messages (currentPeer->msg);
	return true;
}

struct peer *
get_list_head()
{
	return head;
}

bool
check_peer_for_messages(const char *id)
{
	// every non-null peer
	// should have pending messages
	// find the peer, gets its messages, frees the peer
	struct peer *currentPeer = get_peer (get_list_head (), id);
	if(currentPeer == NULL){
		return false;
	}
	get_unread_messages(currentPeer);
	currentPeer = delete_peer(currentPeer);
}
