#pragma once
#include "include/mongoose.h"
#include "lib/datastructs.h"
#include "lib/util.h"

struct fileAddr {
	char *host;
	char *port;
	char *path;
	char *name;
	struct fileAddr *next;
};

void initialize_fileupload_structs ();
void destroy_fileupload_structs ();
void send_file(struct data_wrapper *data);
void manage_file_upload (struct data_wrapper *data);
