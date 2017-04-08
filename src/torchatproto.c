#include "libdill.h"
#include "libdillimpl.h"
#include <sys/socket.h>
#include <unistd.h> // close
#include <arpa/inet.h> 
#include <string.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>// offsetof
#include <ctype.h> // isdigit
#include <fcntl.h>
#include <assert.h>
#include "include/mem.h" // FREE

#include "include/fd.h"
#include "include/utils.h"
#include "lib/torchatproto.h"
#include "lib/util.h"

#define CONT(ptr, type, member) ((type*)(((char*) ptr) - offsetof(type, member)))

// TODO: clean this mess

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
	
	int rc;
	while (1) {
		rc = recv (self->sock, first->iol_base, 2000, 0); 

		if (rc == -1 && errno != EWOULDBLOCK && errno != EAGAIN) {
        	// non blocking socket, 
        	// recv can fail because it would have waited for data
        	// continue iterating, wait for data with fdin
            if(errno == EPIPE) errno = ECONNRESET;
            return -1;
        } else if ((int) rc >= (int) sz) {
				break;
		}
	 	// fdin used only if recv failed
	 	int result = fcntl(self->sock, F_SETFL, O_NONBLOCK); assert (result == 0);
		fdin (self->sock, deadline);
    	/*int rc = fd_recv (self->sock, &fdBuf, first, last, deadline); // fill buffer for each iterator*/
    }
    return rc;
}

int
torchatproto_hdone(struct hvfs *hvfs, int64_t deadline)
{
	return 0;
}

int
torchatproto_fd_unblock(int s)
{
	// set socket to non blocking
	// almost equal to libdill:fd.c:fd_unblock
    /* Switch to non-blocking mode. */
    int opt = fcntl(s, F_GETFL, 0);
    if (opt == -1)
        opt = 0;
    int rc = fcntl(s, F_SETFL, opt | O_NONBLOCK);
    assert(rc == 0);
    /*  Allow re-using the same local address rapidly. */
    opt = 1;
    rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
    assert(rc == 0);
    /* If possible, prevent SIGPIPE signal when writing to the connection
        already closed by the peer. */
    return 0;
}
int 
torchatproto_attach (int s)
{
	// allocates functions inside table
	// returns an handle to the FT
	int err;
	struct torchatproto *self = MALLOC (sizeof (struct torchatproto));
	if (!self) {err = ENOMEM; goto error1;}

	self->hvfs.query = torchatproto_hquery;
	self->hvfs.close = torchatproto_hclose;
	self->hvfs.done = torchatproto_hdone;
	self->mvfs.msendl = torchatproto_msendl;
	self->mvfs.mrecvl = torchatproto_mrecvl;
	self->sock = s;	
	/*torchatproto_fd_unblock (self->sock); // set unblocking recv*/ // should already be set
	int h = hmake (&self->hvfs);
	if (h < 0) {err = errno; goto error2;}
 	return h;
error2:
	FREE (self);
error1:
	errno = err;
	return -1;
}

void
torchatproto_hclose (struct hvfs *hvfs)
{
	// close and FREE
	struct torchatproto *self = (struct torchatproto *) hvfs;
	FREE (self);
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
	FREE(self);
	fdclean (sock);
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
	struct iolist *it = (struct iolist *) MALLOC (sizeof (struct iolist));
	char *buf = CALLOC (sz, sizeof (char));
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
	// then FREE the lists
	while (iol) {
		memcpy (buf, iol->iol_base, iol->iol_len);
		buf = buf + iol->iol_len;
		struct iolist *t = iol;
		iol = iol->iol_next;
		FREE (t->iol_base); FREE (t);
	}
}

static int 
fd_recv_dimensionhead (struct msock_vfs *mvfs, int64_t deadline)
{
	// this function is used to get the dimension from the first 4 bites of the packet
    struct torchatproto *self = CONT(mvfs, struct torchatproto, mvfs);
    char buf[5] = {0}; buf[4] = '\0';
    ssize_t sz = recv(self->sock, buf, 4, 0);
    if(sz == 0) {errno = EPIPE; return -1;}
    if(sz < 0) {
        if(errno != EWOULDBLOCK && errno != EAGAIN) {
            if(errno == EPIPE) errno = ECONNRESET;
            return -1;
        }
            sz = 0;
    }
    for (size_t i = 0; i < 4; ++i) {
    	// check that buf is not garbage else atoi undefined behaviour
    	if (!isdigit (buf[i])) return -1;
    }
    return strtol (buf, NULL, 10);
   	
    // atoi returns 0 on fail
}

ssize_t 
torchatproto_mrecv (int h, void *buf, size_t maxLen, int64_t deadline)
{
	// redefinition of libdill mrecv function for torchat:
	// recv dimension of buffer then the buffer
	// len is size of buf, num of recv bytes will be obtained by a first recv
    struct msock_vfs *m = hquery(h, msock_type);
    struct torchatproto *self = CONT(m, struct torchatproto, mvfs);
        if(!m) { errno = EBADF; return -1; }

	// receive dimension of the buffer 
    int len = fd_recv_dimensionhead (m, deadline);

    if (len <= 0) return -1; // not an integer
    // check if supplied buffer too small
    if ((size_t) len > maxLen) {errno = EMSGSIZE; return -1;}

    // if dimension recvd, proceed to put buffer in iolists
	// allocate first element
	size_t defsz = 256;
	int rc;
	while (1) {
		// actually,
		// with epoll, it should exit this loop after only one iteration
		// TODO remove loop
		rc = recv (self->sock, buf, len, 0); 

		if (rc == -1 && errno != EWOULDBLOCK && errno != EAGAIN) {
        	// non blocking socket, 
        	// recv can fail because it would have waited for data
        	// continue iterating, wait for data with fdin
            if(errno == EPIPE) errno = ECONNRESET;
            return -1;
        } else if ((int) rc >= (int) len) {
				break;
		}
		// socket not ready yet
		// yield control
		yield (); // read above
		if (now () > deadline) {
			assert (false); // should not happen
		}
    }
    return rc;
}


static void
generate_dimension_head (char *buf, size_t len)
{
	// fill buffer with the dimension of the packet to be sent
	// zeroes at beginning
	for (size_t i = 0; i < 4; ++i) {
		buf[3 - i] = len % 10 + '0';
		len = len / 10;
	}
}

ssize_t 
torchatproto_msend (int h, void *buf, size_t len, int64_t deadline)
{
	// redefinition of libdill msend function for torchat:
	// send dimension of buffer then the buffer
    // if len > strlen (buf) bad things are gonna happen
    struct msock_vfs *m = hquery(h, msock_type);
    if(!m) return -1;

    // first compute dimensions
    // send all in a single operation because epoll
    char tbuf[4 + len];
    generate_dimension_head (tbuf, len);
    memcpy (tbuf + 4, buf, len);

    struct iolist iol = {(void*) tbuf, len + 4, NULL, 0};
    int rc = m->msendl (m, &iol, &iol, deadline);
    return rc;
}
