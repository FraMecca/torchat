#include "include/uthash/src/uthash.h"
#include "lib/tc_streams.h"

static struct vfsTable_t *head = NULL;

void
insert_handle (struct vfsTable_t *t)
{
	HASH_ADD_INT (head, fd, t);
}

void
remove_handle (struct vfsTable_t *t)
{
	HASH_DEL (head, t);
}

struct vfsTable_t *
get_handle (int k)
{
	struct vfsTable_t *t;
	HASH_FIND_INT (head, &k, t);
	return t; // null if not exist
}
