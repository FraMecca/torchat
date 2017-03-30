#include "libdill.h"
#include <stdio.h>
#include "libdillimpl.h"
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h> // close
#include <arpa/inet.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <signal.h>
#include <stddef.h>// offsetof
#include <errno.h>
#include <assert.h> // assert

#include "include/fd.h"
#include "lib/torchatproto.h"

#define CONT(ptr, type, member) ((type*)(((char*) ptr) - offsetof(type, member)))

struct torchatproto {
	// handler to the virtual function table
	struct hvfs hvfs;
	struct msock_vfs mvfs;
	int sock;
};

// unique protocol type because of memory address
static const int torchatprotoTypePlaceholder = 0;
static const void *torchatprotoType = &torchatprotoTypePlaceholder;

// prototypes inside function table handler
void *torchatproto_hquery (struct hvfs *hvfs, const void *id);
void torchatproto_hclose (struct hvfs *hvfs);
int torchatproto_hdone (struct hvfs *hvfs, int64_t deadline);
static int torchatproto_msendl(struct msock_vfs *mvfs, struct iolist *first, struct iolist *last, int64_t deadline);
static ssize_t torchatproto_mrecvl(struct msock_vfs *mvfs, struct iolist *first, struct iolist *last, int64_t deadline);

static int 
torchatproto_msendl(struct msock_vfs *mvfs, struct iolist *first, struct iolist *last, int64_t deadline)
{
    struct torchatproto *self = CONT(mvfs, struct torchatproto, mvfs);

	size_t sz = 0;
	struct iolist *it;
	for(it = first; it; it = it->iol_next) {
    	sz += it->iol_len;
    }
	if(sz > 8192) {errno = EMSGSIZE; return -1;}
	// now send the message
	return fd_send (self->sock, first, last, deadline);
}

static ssize_t 
torchatproto_mrecvl(struct msock_vfs *mvfs, struct iolist *first, struct iolist *last, int64_t deadline)
{
    struct torchatproto *self = CONT(mvfs, struct torchatproto, mvfs);

	size_t sz = 0;
	struct iolist *it;
	for(it = first; it; it = it->iol_next) {
    	sz += it->iol_len;
    }
	if(sz > 8192) {errno = EMSGSIZE; return -1;} // 8K is sizemax
	
	struct fd_rxbuf fdBuf;
	fd_initrxbuf (&fdBuf);
    return fd_recv (self->sock, &fdBuf, first, last, deadline); // fill buffer for each iterator
}

int
torchatproto_hdone(struct hvfs *hvfs, int64_t deadline)
{
	return 0;
}

int 
torchatproto_attach (int s)
{
	// allocates functions inside table
	// returns an handle to the FT
	int err;
	struct torchatproto *self = malloc (sizeof (struct torchatproto));
	if (!self) {err = ENOMEM; goto error1;}

	self->hvfs.query = torchatproto_hquery;
	self->hvfs.close = torchatproto_hclose;
	self->hvfs.done = torchatproto_hdone;
	self->mvfs.msendl = torchatproto_msendl;
	self->mvfs.mrecvl = torchatproto_mrecvl;
	self->sock = s;	
	fd_unblock (self->sock); // set unblocking recv
	int h = hmake (&self->hvfs);
	if (h < 0) {err = errno; goto error2;}
 	return h;
error2:
	free (self);
error1:
	errno = err;
	return -1;
}

void
torchatproto_hclose (struct hvfs *hvfs)
{
	// close and free
	struct torchatproto *self = (struct torchatproto *) hvfs;
	free (self);
}

void *
torchatproto_hquery (struct hvfs *hvfs, const void *type)
{
	// checks the type of protocol
	// returns the handler if protocol supported
	struct torchatproto *self = (struct torchatproto *) hvfs;
	if (type == torchatprotoType) return self;
	if (type == msock_type)	return &self->mvfs;
	// else type is not valid
	errno = ENOTSUP;
	return NULL;
}

int 
torchatproto_detach(int h)
{
	// check funct
	struct torchatproto *self = hquery(h, torchatprotoType);	
	if(!self) return -1;
	int sock = self->sock;
	/*self->hvfs.close(self->hvfs);*/
	free(self);
	return sock;
}

int
socket_create(int port, int64_t deadline)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { 
    	return -1;
    }
	struct sockaddr_in servAddr;
	
	bzero((char *) &servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY; // todo: bind to a selected interface
	servAddr.sin_port = htons(port);
	
	/*connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));*/
	int rc = fd_connect (sock, (struct sockaddr *)&servAddr, sizeof(servAddr), deadline);
	if (rc >= 0) return sock; // if connect is successfull, return socket
	return -1; // else return error, errno already set
}

static struct iolist *
allocate_iol_struct (size_t sz)
{
	// allocate one iolist on heap
	// TODO: use the stack
	struct iolist *it = (struct iolist *) malloc (sizeof (struct iolist));
	char *buf = calloc (sz, sizeof (char));
	it->iol_base = (void *) buf;
	it->iol_len = sz;
	it->iol_next = NULL;
	it->iol_rsvd = 0;
	return it;
}

static struct iolist *
torproto_create_iolist (struct iolist *iol, size_t len, size_t sz)
{
	// create the iolists depending on size
	struct iolist *it = iol;
	while ((int) len > 0) {
		len -= sz; // 256 ?
		it->iol_next = allocate_iol_struct(len > sz ? sz : len); 
		// if len is less than sz, allocate the remaining bytes < 256
		it = it->iol_next;
	}
	return it;
}

static void
put_iol_in_buf (struct iolist *iol, void *buf)
{
	// assuming buf is preallocated
	// put all the content of the various buffer of the iolists
	// in buf 
	// then free the lists
	while (iol) {
		memcpy (buf, iol->iol_base, iol->iol_len);
		buf = buf + iol->iol_len;
		struct iolist *t = iol;
		iol = iol->iol_next;
		free (t->iol_base); free (t);
	}
}

ssize_t 
torchatproto_mrecv (int h, void *buf, size_t maxLen, int64_t deadline)
{
	// redefinition of libdill mrecv function for torchat:
	// recv dimension of buffer then the buffer
	// len is size of buf, num of recv bytes will be obtained by a first recv
    struct msock_vfs *m = hquery(h, msock_type);
    if(!m) return -1;

	// receive dimension of the buffer 
    char blen[4] = {'\0'}; // always pad the blen
    struct iolist tiol = {(void*)blen, 4, NULL, 0};
    int rc = m->mrecvl (m, &tiol, &tiol, deadline);
    if (rc < 0) return -1; // can't recv, errno already set
    size_t len = atoi (blen);
    // check if supplied buffer too small
    if (len > maxLen) {errno = EMSGSIZE; return -1;}

    // if dimension recvd, proceed to put buffer in iolists
	// allocate first element
	size_t defsz = 256;
	struct iolist *first, *last;
	if (len > defsz) {
		first = allocate_iol_struct (defsz);
		len -= defsz;
		// allocate remaining elements
		last = torproto_create_iolist (first, len, defsz);
	} else {
		// the whole message fits in one iolist
		first = allocate_iol_struct (len);
		last = first;
	}

    int rsz = m->mrecvl(m, first, last, deadline);
	
	put_iol_in_buf (first, buf); // concatenate iolist buffers it in buf
    return rsz;
}

ssize_t 
torchatproto_msend (int h, void *buf, size_t len, int64_t deadline)
{
	// redefinition of libdill msend function for torchat:
	// send dimension of buffer then the buffer
    // if len > strlen (buf) bad things are gonna happen
    struct msock_vfs *m = hquery(h, msock_type);
    if(!m) return -1;

    // first send dimensions
    char blen[4] = {'\0'};
    snprintf (blen, 4, "%lu", len); // store dimension in blen
    struct iolist tiol = {(void*)blen, 4, NULL, 0};
    int rc = m->msendl (m, &tiol, &tiol, deadline);

    if (rc >= 0) { // if dimension sent
    	/*then send the buffer*/
    	struct iolist iol = {(void*)buf, len, NULL, 0};
    	rc = m->msendl (m, &iol, &iol, deadline);
    }
    return rc;
}
