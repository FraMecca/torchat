#include "lib/file_upload.h"
#include "lib/datastructs.h"
#include "include/mem.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
// these are for file upload functions, the other for file recv
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>

static int nosaveFlag = 0; // used to determine if tmpfile has to be removed or not

struct file_writer_data {
  FILE *fp;
  size_t bytes_written;
};

// FILE UPLOAD FUNCTIONS

struct fileAddr *
load_info(struct data_wrapper *data, struct fileAddr *file)
{
	// this function loads informations about files received by the client
	// puts them in a data structure defined in file_upload.h
	// the handler to the ds is in main.c
	struct fileAddr *newFile;
	char host[30];
	char port[6];
	char fPath[200];
	char fName[100];

	sscanf(data->msg, "%s %s %s %s", fPath, fName, port, host);

	newFile = MALLOC(sizeof(struct fileAddr));
	newFile->host = STRDUP(host);
	newFile->port = STRDUP(port);
	newFile->path = STRDUP(fPath);
	newFile->name = STRDUP(fName);

	if(!file){
		file = newFile;
		file->next = NULL;
	} else {
		newFile->next = file;
		file = newFile;
	}
	return file;
}

struct fileAddr *
next_file(struct fileAddr *fList)
{
	struct fileAddr *file = fList;
	FREE(file->host);
	FREE(file->port);
	FREE(file->name);
	FREE(file->path);
	FREE(file);
	return file->next;
}

char *
build_dest_addr(char *host, char *port)
{
	// build the curl destination address
	char *addr;
	if(!(addr = calloc(strlen(host) + strlen(port) + 10, sizeof(char)))){
		exit(1);
	}

	strncpy(addr, host, strlen(host));
	strncat(addr, ":", 1);
	strncat(addr, port, strlen(port));
	strncat(addr, "/upload", 7);
	return addr;
}

void
post_curl_req(struct fileAddr *file)
{
	// this sends the files through a POST request (http)
	// the form is built by libcurl
	// the filename is the name you would give to the file sent
	// the filepath is the absolute path to the file
	CURL *curl;
	CURLcode res;
	char *dest_addr;
	FILE *fd;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;

	// format arguments
	dest_addr = build_dest_addr(file->host, file->port);

	if(!(fd=fopen(file->path, "rb"))){
		exit_error("file not found");
	}

	// Fill in the file upload field 
	  curl_formadd(&formpost,
			   &lastptr,
			   CURLFORM_COPYNAME, file->name,
			   CURLFORM_FILE, file->path,
			   CURLFORM_END);
	// fill in the filename field
	curl_formadd(&formpost,
			   &lastptr,
			   CURLFORM_FILENAME, file->name,
			   CURLFORM_COPYNAME, file->name,
			   CURLFORM_COPYCONTENTS, file->path,
			   CURLFORM_END);

	curl = curl_easy_init();
	if(curl){
		//upload endpoint
		curl_easy_setopt(curl, CURLOPT_URL, dest_addr);
		//set SOCKS5 proxy
		curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:9250");
		curl_easy_setopt(curl, CURLOPT_PORT, 8000);
		curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
		//enable uploading to endpoint
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

		//enable verbose output
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// send request
		if((res = curl_easy_perform(curl)) != CURLE_OK){
			exit_error("unable to perform POST request");
		}
		curl_easy_cleanup(curl);
	}
	fclose(fd);
	FREE(dest_addr);
}

void *
send_file_routine(void *fI)
{
	// after the connection to the port is established
	// iterate on the list of files 
	// upload them via POST req
	// go on until no files are present
	// retrieve the infos on the current files to send
	struct fileAddr *file = (struct fileAddr *) fI;
	while(file){
		post_curl_req(file);
		file = next_file(file);
	}
}

void send_file(struct fileAddr *file)
{
	// multiple threads to avoid blocking on large files
	// detached to prevent leaks
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
	pthread_create(&t, &attr, &send_file_routine, (void*)file);
}


// FROM HERE DOWNWARDS: ONLY FILE RECV FUNCTION

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
		  FREE(data);
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
file_upload_poll (void *rp)
{
	// create a new connection on the port advertised
	// poll for the file to be transfered
	char *port = (char*) rp;
 	bool file_received = false;
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);  // Initialize event manager object

    struct mg_connection *nc; 
    nc = mg_bind(&mgr, port, handle_upload);  // Create listening connection and add it to the event manager
	
	// Add http events management to the connection
	mg_set_protocol_http_websocket(nc);

	while (!file_received) {
        mg_mgr_poll(&mgr, 300);
    }
    // close connection TODO
    // and FREE resources TODO
    return 0;
}

static void
advertise_port (struct data_wrapper *data, char *port)
{
	// puts the random port in the data field so that it can be sent to the other peer's server
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
	/*int p = rand () % (65535 - 2048) + 2048;*/
	int p = 43434;
	char *port = MALLOC(6*sizeof(char));
	snprintf (port, sizeof (port), "%d", p);

	// advertise port
	advertise_port (data, port); // just fill data_wrapper with information on port // main.c sends the msg

	// start polling on the new connection for the file (on a separate thread)
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
	printf("%s", port);
	pthread_create(&t, &attr, &file_upload_poll, (void*)port);
	return;
}
