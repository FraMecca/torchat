#include "lib/file_upload.h"
#include "lib/datastructs.h"
#include "include/mem.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
// these are for file upload functions, the other for file recv
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h> // inet
#include <netinet/in.h> // socket types
#include <errno.h> // perror

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

static bool 
set_socket_timeout (const int sockfd)
{
	// this function sets the socket timeout
	// 120s
	    struct timeval timeout;      
	    timeout.tv_sec = 120;
	    timeout.tv_usec = 0;

	    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
	        perror("setsockopt failed\n");
	        return false;
	    }

	    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
	        perror("setsockopt failed\n");
	        return false;
	    }
	    return true;
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
<<<<<<< HEAD
	struct fileAddr *file = fList->next;
	FREE(fList->host);
	FREE(fList->port);
	FREE(fList->name);
	FREE(fList->path);
	FREE(fList);
	return file;
}

int
send_file_over_tor(struct fileAddr *file, const int torPort)
{ 
	// buf is the json parsed char to be sent over a socket
    int sock;
    struct sockaddr_in socketAddr;
	
	char *domain = STRDUP(file->host);
	int portno = file->port;
	/*int portno = 80;*/

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (!set_socket_timeout (sock)) {
    	perror ("setsockopt");
    	exit (1);
    }
    socketAddr.sin_family       = AF_INET;
    socketAddr.sin_port         = htons(torPort);
    socketAddr.sin_addr.s_addr  = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr*)&socketAddr, sizeof(struct sockaddr_in));

    char handShake[3] =
    {
        0x05, // SOCKS 5
        0x01, // One Authentication Method
        0x00  // No AUthentication
    };
    send(sock, handShake, 3, 0);

    char resp1[2];
    recv(sock, resp1, 2, 0);
    if(resp1[1] != 0x00)    {
        return 9; // Error, handshake failed ?
    }

    char  domainLen = (char)strlen(domain);
    short port = htons(portno);

    char TmpReq[4] = {
          0x05, // SOCKS5
          0x01, // CONNECT
          0x00, // RESERVED
          0x03, // domain
        };

    char* req2 = malloc ((4+1+domainLen + 2) *sizeof (char));

    memcpy(req2, TmpReq, 4);                // 5, 1, 0, 3
    memcpy(req2 + 4, &domainLen, 1);        // Domain Length
    memcpy(req2 + 5, domain, domainLen);    // Domain
    memcpy(req2 + 5 + domainLen, &port, 2); // Port

    send(sock, (char*)req2, 4 + 1 + domainLen + 2, 0);
    free (req2);

    char resp2[10];
    recv(sock, resp2, 10, 0);
    // if resp2 is ok, tor opened a socket to domain (.onion)
	if(resp2[1] != 0){
		return resp2[1];
	}
	int	fd = open(file->path, O_RDONLY);
	printf("opened file");
	while(sendfile(sock, fd, NULL, 4096) > 0);
	printf("file sent");
	return 0;
=======
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
>>>>>>> master
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
<<<<<<< HEAD
	while(file){
		send_file_over_tor(file, 9250);			
		file = next_file(file);
=======
	if (file){
		post_curl_req(file);
		next_file(file);
>>>>>>> master
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

<<<<<<< HEAD
=======
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

>>>>>>> master
static void *
file_upload_poll ()
{
	// create a new connection on the port advertised
	// poll for the file to be transfered
<<<<<<< HEAD
	char *port = (char*) rp;
 	bool file_received = false;
	int socket_desc , client_sock , c , read_size;

    struct sockaddr_in server, client;
    char client_message[2000];
   
	//Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(port));	

	//Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
   
    //Listen
    listen(socket_desc , 3);
 
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    //accept connection from an incoming client
    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    puts("Connection accepted");
     
    //Receive a message from client
    while( (read_size = recv(client_sock , client_message , 4096 , 0)) > 0 ){
		printf("eccolo");
	}
    return 0;
=======
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
>>>>>>> master
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

<<<<<<< HEAD
	// select random portrange
	srand (time (NULL));
	/*int p = rand () % (65535 - 2048) + 2048;*/
	int p = 8888;
	char *port = MALLOC(6*sizeof(char));

	snprintf (port, sizeof (port), "%d", p);
=======
>>>>>>> master

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
