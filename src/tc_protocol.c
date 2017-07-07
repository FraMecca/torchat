#include "lib/tc_sockets.h"
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include "lib/tc_streams.h"
#include "lib/tc_protocol.h"

int
tc_message_attach (int fd)
{
	struct vfsTable_t *t;

	t = malloc (sizeof (struct vfsTable_t));
	t->tc_type = MESSAGE_T;
	t->tc_proto_send = tp_msend;
	t->tc_proto_close = tp_mclose;
	t->tc_proto_recv = tc_mrecv;
	t->tc_data = NULL;
	t->fd = fd;
    
    int ret = t->fd;
    insert_handle (t);
	return ret;
}

int
tp_msend (struct vfsTable_t *t, char *buf, size_t len)
{
	char buf2[2+len];
	buf2[0] = len & 0xFF;
	buf2[1] = len >> 8;
	memcpy (buf2 + 2, buf, len);

	int rc = write (t->fd, buf2, len + 2);
	return rc;
}

int
tc_mrecv (struct vfsTable_t *t, char *buf)
{
	// TODO, inizia ad usarla, sposta buf nel prototipo
	char tbuf[2];
	int rc = read (t->fd, tbuf, 2);
	if (rc == -1) return rc;
	uint16_t size = tbuf[0] | tbuf[1] << 8;	
	buf = calloc (size,sizeof (char));
	rc = read (t->fd, buf, size);
	return rc;
}

int
tp_mclose (struct vfsTable_t *t)
{
	// this function close connection
	// and send close json to receiving node
	close (t->fd);
	// no check for error
	return 0;
}

