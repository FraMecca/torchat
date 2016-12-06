#include "../lib/datastructs.h" // struct list
#include <string.h> // strdup
#include <stdlib.h> // malloc
#include "../lib/util.h" // get_date
#include <stdbool.h>

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
	
	new->next = head;
	head->prev = new;
	return new;
}

static struct peer *
get_peer (struct peer *current, const char *id)
{
	// get the node of a peer 
	// by iterating on the list
	// returns null if the id is not found
	//
	// needs head because recursion
	if (head == NULL) {
		return NULL;
		// id not found
	} else if (strcmp (current->id, id) == 0) {
		return head;
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
	struct message *msgHead;
	msgHead = p->msg;

	msgHead->prev = new;
	new->next = msgHead;
	return new;
}
