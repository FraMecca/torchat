#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "lib/tc_handle.h"
#include "lib/tc_mem.h"
#include "lib/tc_sockets.h"
#include "lib/tc_util.h"
#include "lib/tc_messages.h"
#include "lib/tc_client.h"
#include "lib/tc_json.h"

struct peersTable_t;
void peer_insert_handle (struct peersTable_t *p);
void peer_remove_handle (struct peersTable_t *p);
struct peersTable_t *peer_query (char *k);
struct peersTable_t * create_peerTable (char *id, int sock);
void tc_destroy_client_data ();

int
tc_client_attach (int fd)
{
	// load vtf table with functions 
	// that operates on message streams
	struct vfsTable_t *t;

	t = malloc (sizeof (struct vfsTable_t));
	t->tc_type = CLIENT_T;
	t->send = tc_csend;
	t->close = tc_cclose;
	t->recv = tc_crecv;
	t->tc_data = NULL;
	t->fd = fd;
   
    int ret = t->fd;
    tc_insert_handle (t);
	return ret;
}

/*static struct tc_message_t **/
/*tc_message_build (JSON *j)*/
/*{*/
	/*struct tc_message_t *m = malloc (sizeof (struct tc_message_t));*/
	/*m->id = strdup((char *) get (j, "from"));*/
	/*m->message = strdup((char *) get (j, "message"));*/
	/*return m;*/
/*} */

int
tc_csend (int fd, JSON *j)
{
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

static JSON *
create_message_jmu (JSON *src, int port)
{
	JSON *dst = JSON_new ();
	JSON_add_int (dst, "port", port);
	JSON_add_str (dst, "from", "DEMO");

	return dst;
}

int
tc_crecv (int fd)
{
	// implement recv for torchat client json units
	// TODO: AUTH
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
	assert (rc == -1 || size == rc);
	// act on command TODO
	// should switch on commands
	char *id = (char *) json_get (j, "to");
	int port = *(int *)json_get (j, "port");
	JSON *nj = create_message_jmu (j, port);
	struct peersTable_t *p = peer_query (id);
	if (p == NULL) {
		int nfd = fd_connect (id, port);
		peer_insert_handle (create_peerTable (id, nfd));
		p = peer_query (id);
	}
	p->t->send (p->t->fd, nj); 
	//
	return rc;
}


int
tc_cclose (int fd)
{
	// this function close connection
	// and send close json to receiving node
	struct vfsTable_t *t = tc_query (fd);
	close (t->fd);
	// no check for error
	return 0;
}

/************* HASH TABLE *************/
// map id -> vtfs_table
struct peersTable_t {
	struct vtfs_table_t *t;
	char *id;
};

struct peersTable_t *
create_peerTable (char *id, int sock)
{
	struct peersTable_t *p = malloc (sizeof (struct peersTable_t));
	p->id = strdup (id);
	// TODO: not only for tc_messages 
	struct vtfs_table_t *t =  tc_attach (sock);
	p->t =  t;
	return p;
}

static struct peersTable_t *head = NULL; // use an hash table

void
peer_insert_handle (struct peersTable_t *p)
{
	HASH_ADD_STR (head, id, t);
}

void
peer_remove_handle (struct peersTable_t *p)
{
	HASH_DEL (head, p);
}

struct peersTable_t *
peer_query (char *k)
{
	struct peersTable_t *p;
	HASH_FIND_INT (head, &k, p);
	return p; // null if not exist
}

void
tc_destroy_client_data ()
{
	struct peersTable_t *ptr, *p;

	if (head == NULL) return;
	HASH_ITER (hh, head, p, ptr) {
		HASH_DEL (head, p);
		free (p->id);
		free (p);
	}
}
