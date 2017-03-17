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
static char port[] = "43434";
static pthread_mutex_t pollMut;
extern void log_err (char *json); // from logger.cpp

struct file_writer_data {
  FILE *fp;
  size_t bytes_written;
};

// FILE UPLOAD FUNCTIONS
void 
initialize_fileupload_structs ()
{
	if (pthread_mutex_init (&pollMut, NULL) != 0) {
		exit_error ("Can't initialize file_upload_poll_mutex");
	}
}

void 
destroy_fileupload_structs ()
{
	pthread_mutex_destroy (&pollMut);
}

struct fileAddr *
load_info(struct data_wrapper *data)
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

	return newFile;
}

struct fileAddr *
next_file(struct fileAddr *fList)
{
	if (fList){
		FREE(fList->host);
		FREE(fList->port);
		FREE(fList->name);
		FREE(fList->path);
		FREE(fList);
	}
	return NULL;
}

char *
build_dest_addr(char *host, char *portno)
{
	// build the curl destination address
	char *addr;
	if(!(addr = calloc(strlen(host) + strlen(portno) + 10, sizeof(char)))){
		exit(1);
	}

	strncpy(addr, host, strlen(host));
	strncat(addr, ":", 1);
	strncat(addr, port, strlen(portno));
	/*strncat(addr, "/upload", 7);*/
	return addr;
}

static void
post_curl_req(struct fileAddr *file)
{
	// this sends the files through a POST request (http)
	// the form is built by libcurl
	// the filename is the name you would give to the file sent
	// the filepath is the absolute path to the file
	CURL *curl;
	/*CURLcode res;*/
	char *dest_addr;
	char *err;
	FILE *fd;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;

	if(!(fd=fopen(file->path, "rb"))){
		err = MALLOC((20+strlen(file->path))*sizeof(char));
		sprintf(err, "File %s not found.", file->path);
		log_err(err);
		FREE(err);
		return;
	}

	// format arguments
	dest_addr = build_dest_addr(file->host, file->port);
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
		curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);
		//enable uploading to endpoint
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

		//enable verbose output
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// send request, does not check for return value
		// at this point http delivers even if errors occour
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	fclose(fd);
	FREE(dest_addr);
	/*curl_formfree(formpost);*/
	/*curl_formfree(lastptr);*/
	curl_global_cleanup();
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
	if (file){
		post_curl_req(file);
		next_file(file);
	}
	pthread_exit (NULL);
}

void 
send_file(struct data_wrapper *data)
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
	struct fileAddr *file = load_info (data);
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
			} else {
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
file_upload_poll ()
{
	// create a new connection on the port advertised
	// poll for the file to be transfered
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);  // Initialize event manager object

    struct mg_connection *nc; 
	// Create listening connection and add it to the event manager

    if((nc = mg_bind(&mgr, port, handle_upload)) == NULL) {
    	exit_error ("Unable to bind to the given port");
    }


	// Add http events management to the connection
	mg_set_protocol_http_websocket(nc);

	while (true) {
        mg_mgr_poll(&mgr, 300);
    }
    // close connection 
    mg_mgr_free(&mgr); // terminate mongoose connection
    pthread_exit (NULL);
}

static void
advertise_port (struct data_wrapper *data)
{
	// puts the port in the data field so that it can be sent to the other peer's server
	data->cmd = FILEPORT;
	FREE (data->msg);
	data->msg = STRDUP (port);
}

void
manage_file_upload (struct data_wrapper *data)
{
 	// bind to torrc second port
 	// send a json confirming the acceptance and the selected port
 	// wait for file with the http ev handler


	// advertise port
	advertise_port (data); // just fill data_wrapper with information on port // main.c sends the msg
	if (pthread_mutex_trylock (&pollMut) != 0) {	
		// if mongoose is already polling do not start another thread
		return;
	} 

	// else
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
	pthread_create(&t, &attr, &file_upload_poll, NULL);
	return;
}
