#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#include "lib/tc_handle.h"
#include "lib/tc_mem.h"
#include "lib/tc_sockets.h"
#include "lib/tc_util.h"
#include "lib/tc_messages.h"
#include "lib/tc_json.h"

/* MESSAGE TYPES
 * all the functions to handle messages in torchatproto
 */

/************* QUEUE *************/

struct tc_message_t {
	char *id;
	char *message;
};

struct tc_queue_t {
	struct tc_message_t *data;
	struct tc_queue_t *prev;
};

static struct tc_queue_t *head = NULL;
static struct tc_queue_t *tail = NULL;

static void
enqueue (struct tc_message_t *m)
{
	struct tc_queue_t *q = malloc	(sizeof (struct tc_queue_t));
	if (head == NULL){
		head = q;
	}
	q->prev = tail;
	q->data = m;
	tail = q;
}

static struct tc_queue_t *
dequeue ()
{
	struct tc_queue_t *q = tail;
	tail = tail->prev;
	return q;
}

static void
destroy_message (struct tc_message_t *m)
{
	free (m->id);
	free (m->message);
	free (m);
}

void
tc_messages_destroy ()
{
	struct tc_queue_t *tmp;
	while (tail){
		tmp = tail;
		tail = tail->prev;
		destroy_message (tmp->data);
		free (tmp);
	}
}

/************* MESSAGE ************/

int
tc_message_attach (int fd)
{
	// load vtf table with functions 
	// that operates on message streams
	struct vfsTable_t *t;

	t = malloc (sizeof (struct vfsTable_t));
	t->tc_type = MESSAGE_T;
	t->send = tc_msend;
	t->close = tc_mclose;
	t->recv = tc_mrecv;
	t->tc_data = NULL;
	t->fd = fd;
   
    int ret = t->fd;
    tc_insert_handle (t);
	return ret;
}

static struct tc_message_t *
tc_message_build (JSON *j)
{
	struct tc_message_t *m = malloc (sizeof (struct tc_message_t));
	m->id = strdup((char *) json_get (j, "from"));
	m->message = strdup((char *) json_get (j, "message"));
	return m;
} 

static bool
check_json_validity (JSON *j)
{
	if (json_get (j, "from") == NULL || json_get (j, "message") == NULL || 
			json_get (j, "port") > 0) || json_length(j) != 3){
		return false;
	}
	return true;
}

int
tc_msend (int fd, JSON *j)
{
	// check json
	if (!check_json_validity(j)){
		errno = EOPNOTSUPP;
		return -1;
	}
	// implement send for torchat message json units
	autofree char *buf = json_dump (j);
	size_t len = strlen (buf);
	struct vfsTable_t *t = tc_query (fd);
	char buf2[2 + len];
	// size at begin of the message
	buf2[0] = len & 0xFF;
	buf2[1] = len >> 8;
	memcpy (buf2 + 2, buf, len);

	int rc = write (t->fd, buf2, len + 2);
	return rc;
}

int
tc_mrecv (int fd)
{
	// implement recv for torchat message json units
	struct vfsTable_t *t = tc_query (fd);
	uint16_t size;
	int rc = read (t->fd, &size, sizeof (uint16_t));
	if (rc <= 0) return rc;
	// now convert size that is noted at begin of the message 
	// and use it to read correctly
	if (size > MSIZEMAX) { errno = EMSGSIZE; return -1; } 
	char buf[MSIZEMAX];
	rc = read (t->fd, buf, size);
	// null terminated for the parse json function
	buf[size] = '\0';
	// parse the buffer, return a datastruct
	customfree(destroy_json) JSON *j = json_parse(buf);
	if (j == NULL) {
		// wrong json format
		errno = EPROTOTYPE;
		return -1;
	}
	enqueue (tc_message_build (j));	
	assert (rc == -1 || size == rc);
	return rc;
}


int
tc_mclose (int fd)
{
	// this function close connection
	// and send close json to receiving node
	struct vfsTable_t *t = tc_query (fd);
	close (t->fd);
	// no check for error
	return 0;
}
