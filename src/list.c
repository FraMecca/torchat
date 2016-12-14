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
delete_message (struct message *msg)
{
	// frees the message and deletes its content
	// delete all messages
	struct message *m = msg->next, *tmp;
	free(msg->content);
	free(msg->date);
	tmp = msg;
	msg = msg->next;
	free(tmp);
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

char *
get_unread_message(const char *peerId)
{
	// for the given peer
	// check if there are messages that need to be read
	// if there are any, return the oldest one
	// otherwise, NULL is returned
	struct peer currentPeer = get_peer(head, peerId);
	if(currentPeer == NULL){
		return NULL;
	}
	struct message *msg = currentPeer->msg;
	if(msg == NULL){
		return NULL;
	}
	int len = strlen(msg->content)+strlen(msg->date)+3;
	char *m = calloc(len, sizeof(char));

	strncpy(m, msg->date, strlen(msg->date));
	strncat(m, ": ", 2);
	strncat(m, msg->content, strlen(msg->content));

	delete_message (currentPeer->msg);
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
	struct peer *ptr = head;
	size_t size = 0; // size = sum of strlen of all peer id
	if (ptr == NULL) {
		return NULL;
	} else {
		while (ptr != NULL) {
			size += strlen (ptr->id) + 1; // +1 is for the comma
			ptr = ptr->next;
		}
		ptr = head; // reset ptr
		char *peerList = calloc (size, sizeof (char));
		while (ptr != NULL) {
			// iterate again and concatenate the char*
			strncat (peerList, ptr->id, strlen (ptr->id));
			strncat (peerList, ",", sizeof (char));
			ptr = ptr->next;
		}
		return peerList; // already in heap
	}
}



/*bool*/
/*check_peer_for_messages(const char *id)*/
/*{*/
	/*// every non-null peer*/
	/*// should have pending messages*/
	/*// find the peer, gets its messages, frees the peer*/
	/*struct peer *currentPeer = get_peer (get_list_head (), id);*/
	/*if(currentPeer == NULL){*/
		/*return false;*/
	/*}*/
	/*get_unread_messages(currentPeer);*/
	/*currentPeer = delete_peer(currentPeer);*/
/*}*/
