#include "../include/uthash/src/uthash.h"

enum type { FILE_T, MESSAGE_T };
struct vfsTable_t {
	int (* send)  (int, void *, size_t );
	int (* recv)  (int); 
	int (* close) (int); 
	int fd;
	enum type tc_type;
	void *tc_data;
	UT_hash_handle hh;
};

void tc_insert_handle (struct vfsTable_t *t);
void tc_remove_handle (struct vfsTable_t *t);
struct vfsTable_t * tc_query (int k);
void tc_destroy_handlers ();
