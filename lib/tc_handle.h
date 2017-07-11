#pragma once
#include "include/uthash/src/uthash.h"
#include "lib/tc_json.h"

enum type { FILE_T, MESSAGE_T , CLIENT_T};
struct vfsTable_t {
	int (* send)  (int, JSON *);
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
