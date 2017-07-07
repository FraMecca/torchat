#include "../include/uthash/src/uthash.h"

enum type { FILE_T, MESSAGE_T };
struct vfsTable_t {
	int (* tc_proto_send) (struct vfsTable_t * , char * , size_t );
	int (* tc_proto_recv) (struct vfsTable_t * , char * ); 
	int (* tc_proto_close) (struct vfsTable_t *); 
	int fd;
	enum type tc_type;
	void *tc_data;
	UT_hash_handle hh;
};

void insert_handle (struct vfsTable_t *t);
void remove_handle (struct vfsTable_t *t);
struct vfsTable_t * get_handle (int k);
