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

struct fileAddr *load_info(struct data_wrapper *data, struct fileAddr *file);
void send_file(struct fileAddr *file);
void manage_file_upload (struct data_wrapper *data);
