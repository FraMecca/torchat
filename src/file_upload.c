#include "lib/file_upload.h"
#include "lib/datastructs.h"
#include "include/mem.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

static int nosaveFlag = 0; // used to determine if tmpfile has to be removed or not

struct file_writer_data {
  FILE *fp;
  size_t bytes_written;
};

static void
handle_upload(struct mg_connection *nc, int ev, void *p) {
	// This function is called from the mg http event parser
	// It is used to save a file to a local directory as soon as
	// an HTTP POST request with target /upload is received
	
	struct file_writer_data *data = (struct file_writer_data *) nc->user_data;
	struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;

	// check io len, see main.c:112
	 switch (ev) {
		case MG_EV_HTTP_PART_BEGIN: {
		  // the first time an uploaded file is received, populate the data struct
		  if (data == NULL) { 
			data = calloc(1, sizeof(struct file_writer_data));
			if(mp->file_name != NULL && strcmp(mp->file_name, "") != 0){
				data->fp = fopen(mp->file_name, "w+b");
				nosaveFlag = 1;
			}else {
				// if a file is not given (should be) open a temporary one.
				// note: this file is actually needed even if a file is given, 
				// prevents SEGVs. If the file name is given, tmpfile is removed
				// at the end of the transfer
				data->fp = fopen("tmpfile", "w+b");	
			}
			data->bytes_written = 0;

			if (data->fp == NULL) {
			  nc->flags |= MG_F_SEND_AND_CLOSE;
			  return;
			}
			nc->user_data = (void *) data;
		  }
		  break;
		}
		case MG_EV_HTTP_PART_DATA: { 
		  // while data chunks are received, write them

		  if (fwrite(mp->data.p, 1, mp->data.len, data->fp) != mp->data.len) {
			nc->flags |= MG_F_SEND_AND_CLOSE;
			return;
		  }
		  data->bytes_written += mp->data.len;
		  break;
		}
		case MG_EV_HTTP_PART_END: {
		  // upload finished, write confirm and close

		  nc->flags |= MG_F_SEND_AND_CLOSE;
		  fclose(data->fp);
		  free(data);
		  nc->user_data = NULL;
		  if(nosaveFlag){
			remove("tmpfile");
			nosaveFlag = 0;
		  }
		  break;
	}
  }
}

static void *
file_upload_poll ()
{
 	bool file_received = false;
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);  // Initialize event manager object

    struct mg_connection *nc; 
    nc = mg_bind(&mgr, "8001", handle_upload);  // Create listening connection and add it to the event manager
	
	// Add http events management to the connection
	mg_set_protocol_http_websocket(nc);

	while (!file_received) {
        mg_mgr_poll(&mgr, 300);
    }
    // close connection
    // and free resources
    return 0;
}

static void
advertise_port (struct data_wrapper *data, char *port)
{
	data->cmd = FILEPORT;
	FREE (data->msg);
	data->msg = STRDUP (port);
}

void
manage_file_upload (struct data_wrapper *data)
{
 	// select a new random port
 	// bind to it
 	// send a json confirming the acceptance and the selected port
 	// wait for file with the http ev handler

	// select random portrange
	srand (time (NULL));
	int p = rand () % (65535 - 2048) + 2048;
	char port[6];
	snprintf (port, sizeof (port), "%d", p);

	// advertise port
	advertise_port (data, port); // just fill data_wrapper with information on port // main.c sends the msg
	pthread_t t;
	pthread_attr_t attr; // create an attribute to specify that thread is detached
	if (pthread_attr_init(&attr) != 0) {
		// initialize pthread attr and check if error
		exit_error ("pthread_attr_init");
	}
	// set detached state
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
	    exit_error ("pthread_attr_setdetachstate");
	}
	pthread_create(&t, &attr, &file_upload_poll, NULL);
	return;
}
