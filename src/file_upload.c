#include "lib/file_upload.h"
#include "lib/datastructs.h"
#include "lib/actions.h"
#include "include/mem.h"
#include "lib/socks_helper.h" // handshake_with_tor
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h> // perror

static char port[] = "43434";
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
struct fileAddr *
load_info(struct data_wrapper *data)
{
	// this function loads informations about files received by the client
	// puts them in a data structure defined in file_upload.h
	// the handler to the ds is in main.c
	struct fileAddr *newFile;
	char host[30];
	char fPath[200];
	char fName[100];

	sscanf(data->msg, "%s %s %s", fPath, fName, host);

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
send_file(struct fileAddr *file)
{
	// send_file_routine is a coroutine (fileactions.h)
   	int sock = handshake_with_tor (9250);
	go(send_file_routine(sock, file));
}

// FROM HERE DOWNWARDS: ONLY FILE RECV FUNCTION

coroutine static void
handle_upload(const int sock)
{
	// It is used to save a file to a local directory as soon as
	// a json with the content encoded in base64 is received

	struct data_wrapper *data = NULL;
	char *json = NULL;
	int64_t deadline = now() + 12000;
	// fill data and json if the connection was valid
	while (parse_connection (sock, &data, &json, deadline) > 0) {
		// the json is valid, switch on cases
		switch (data->cmd) {
			case FILEBEGIN :
				log_info (json);
				create_file (data); 
				// actually just resets file content
				break;
			case FILEDATA :
				log_info (json); // not really needed
				write_to_file (data);
				// doesn't send any ack (no guaranteed delivery)
				break;
			case FILEEND :
				log_info (json);
				// file is being closed after every read
				break;
			default :
				break;
		}
	}
	/*FREE (json);*/
	/*free_data_wrapper (data);*/
	return;
}

static void *
file_upload_poll ()
{
	// bind to the port advertised
	// poll for the file to be transfered
	// start coroutine
	struct ipaddr addr;
    int rc = ipaddr_local(&addr, NULL, atoi(port), 0);
    assert(rc == 0);
    int ls = tcp_listen(&addr, 10);
    if(ls < 0) {
        exit_error("Can't open listening socket");
    }

    while (true) {  // start poll loop
        int sock = tcp_accept(ls, NULL, -1);
        // check errno for tcp_accept
        assert(sock >= 0);
		// clrf means that lines must be \r\n terminated
        sock  = crlf_attach(sock);
		assert(sock >= 0);
        int cr = go(handle_upload(sock));
        assert(cr >= 0);
    }
}

void
manage_file_upload ()
{

	// fork to start polling on port 43434
	// this way the main coroutine is free to deal with the client
	// while a file upload is being processed 
	if(fork() == 0){
		// returns if port is already binded by a child
		// which is already dealing with the file upload
		// TODO set timer for child to live
		// (child should die after a period of inactivity)
		file_upload_poll();
	}
	return;
}
