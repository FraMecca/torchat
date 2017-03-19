#pragma once
#include "include/mongoose.h"
#include "lib/datastructs.h"
#include "lib/util.h"

struct fileAddr {
	char *host;
	char *port;
	char *path;
	char *name;
};

char * get_upload_port ();
void initialize_fileupload_structs ();
void destroy_fileupload_structs ();
void send_file(struct data_wrapper *data);
void manage_file_upload (struct data_wrapper *data);
void * send_file_routine(void *fI);

void
write_to_file ( struct data_wrapper *data);
void
create_file ( struct data_wrapper *data);
