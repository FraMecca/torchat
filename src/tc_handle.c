// This file contains 
// datastructs and related functions to manage the handlers
// of the vtf tables
//

#include "include/uthash/src/uthash.h"
#include <assert.h>
#include "lib/tc_handle.h"

static struct vfsTable_t *head = NULL; // use an hash table

void
tc_insert_handle (struct vfsTable_t *t)
{
	HASH_ADD_INT (head, fd, t);
}

void
tc_remove_handle (struct vfsTable_t *t)
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

struct vfsTable_t *
tc_query (int k)
{
	// query the hash table for a structure
	// if null?
	struct vfsTable_t *t = get_handle (k);
	assert (t != NULL);
	return t;
}

void
tc_destroy_handlers ()
{
	struct vfsTable_t *ptr, *p;

	if (head == NULL) return;
	HASH_ITER (hh, head, p, ptr) {
		HASH_DEL (head, p);
		free (p);
	}
}
