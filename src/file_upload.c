#include "lib/file_upload.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int nosaveFlag = 0; // used to determine if tmpfile has to be removed or not

struct file_writer_data {
  FILE *fp;
  size_t bytes_written;
};

void
handle_upload(struct mg_connection *nc, int ev, void *p) {
	// This function is called from the mg http event parser
	// It is used to save a file to a local directory as soon as
	// an HTTP POST request with target /upload is received
	
	struct file_writer_data *data = (struct file_writer_data *) nc->user_data;
	struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;

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

