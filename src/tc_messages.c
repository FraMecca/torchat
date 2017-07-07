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


int
tc_message_attach (int fd)
{
	// load vtf table with functions 
	// that operates on message streams
	struct vfsTable_t *t;

	t = malloc (sizeof (struct vfsTable_t));
	t->tc_type = MESSAGE_T;
	t->tc_proto_send = tc_msend;
	t->tc_proto_close = tc_mclose;
	t->tc_proto_recv = tc_mrecv;
	t->tc_data = NULL;
	t->fd = fd;
    
    int ret = t->fd;
    insert_handle (t);
	return ret;
}

int
tc_msend (int fd, unsigned char *buf, size_t len)
{
	// implement send for torchat message json units
	struct vfsTable_t *t = tc_query (fd);
	char buf2[4 + len];
	// size at begin of the message
	buf2[0] = len & 0xFF;
	buf2[1] = len >> 8;
	buf2[2] = len >> 16;
	buf2[3] = len >> 24;
	memcpy (buf2 + 4, buf, len);

	int rc = write (t->fd, buf2, len + 4);
	return rc;
}

int
tc_mrecv (int fd, unsigned char *buf)
{
	// implement recv for torchat message json units
	struct vfsTable_t *t = tc_query (fd);
	char tbuf[2];
	int rc = read (t->fd, tbuf, 4);
	if (rc <= 0) return rc;
	// now convert size that is noted at begin of the message 
	uint32_t size = (tbuf[0] - '0') | (tbuf[1] - '0') << 8 | (tbuf[2] - '0') << 16 | (tbuf[3] - '0') << 24 ;
	// and use it to read correctly
	if (size > MSIZEMAX) { errno = EMSGSIZE; return -1; } 
	rc = read (t->fd, buf, size);
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
