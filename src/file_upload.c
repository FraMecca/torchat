#include "lib/file_upload.h"
#include "lib/datastructs.h"
#include "lib/actions.h"
#include "include/mem.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
// these are for file upload functions, the other for file recv
#include <errno.h> // perror

static char port[] = "43434";
static pthread_mutex_t pollMut;
extern void log_err (char *json); // from logger.cpp
extern void log_info (char *json); // from logger.cpp
extern struct data_wrapper *convert_string_to_datastruct (const char *jsonCh); // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper *data); // from json.cpp


/*static char *uploadDir = "uploads/"; // TODO: read it from configs*/
char *
get_upload_port ()
{
	return STRDUP (port);
}

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

void
free_file (struct fileAddr *fList)
{
	FREE(fList->host);
	FREE(fList->port);
	FREE(fList->name);
	FREE(fList->path);
	FREE(fList);
}

void 
send_file(struct data_wrapper *data)
{
	// multiple threads to avoid blocking on large files
	// detached to prevent leaks
	pthread_t t;
	pthread_attr_t attr; // create an attribute to specify that thread is detached
	set_thread_detached (&attr);
	struct fileAddr *file = load_info (data);
	if (file) {
		pthread_create(&t, &attr, &send_file_routine, (void*)file);
	}
}

// FROM HERE DOWNWARDS: ONLY FILE RECV FUNCTION
static void
handle_upload(struct mg_connection *nc)
{
	// It is used to save a file to a local directory as soon as
	// a json with the content encoded in base64 is received

	struct data_wrapper *data = NULL;
	char *json = NULL;
	// fill data and json if the connection was valid
	if (parse_mongoose_connection (nc, &data, &json) == false) {
		// if false, log has already been taken care of
		return;
	}
	// the json is valid, switch on cases
    switch (data->cmd) {
    	case FILEBEGIN :
    		log_info (json);
    		create_file (data); // actually just resets file content
    		break;
    	case FILEDATA :
			log_info (json); // not really needed
            write_to_file (data);
            break;
        case FILEEND :
    		log_info (json);
            /*close_file (data);*/ 
  			// file is being closed after every read
			break;
        default :
			break;
    }

	/*FREE (json);*/
	/*free_data_wrapper (data);*/
	return;
}

static void
ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    if (nc->recv_mbuf.size == 0) {
        // can trash
    } else if (ev == MG_EV_RECV) {
	// now we just utilize MG_EV_RECV because the response must be send over TOR
		handle_upload (nc);
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

    if((nc = mg_bind(&mgr, port, ev_handler)) == NULL) {
    	exit_error ("Unable to bind to the given port");
    }
    // poll for connections
    // TODO add timer
	while (true) {
        mg_mgr_poll(&mgr, 300);
    }
    // close connection 
    mg_mgr_free(&mgr); // terminate mongoose connection
    pthread_mutex_unlock (&pollMut);
    pthread_exit (NULL);
}

void
manage_file_upload (struct data_wrapper *data)
{
 	// bind to torrc second port
 	// send a json confirming the acceptance and the selected port

	if (pthread_mutex_trylock (&pollMut) != 0) {	
		// if mongoose is already polling do not start another thread
		return;
	} 

	// else start and bind mongoose for first time
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
