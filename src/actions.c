#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/mem.h"
#include "lib/datastructs.h"
#include "lib/socks_helper.h"
#include "lib/util.h"
#include <pthread.h>
#include "lib/actions.h"
#include "include/libdill.h"

extern struct data_wrapper * convert_string_to_datastruct (const char *jsonCh);  // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper *data);  // from json.cpp
extern void log_info (char *json); // from logger.cpp
extern void log_err (char *json); // from logger.cpp
extern char * HOSTNAME;
// in this file there are the various functions used by main::event_routine
// related to the various commands
//
//

bool
parse_connection (const int sock, struct data_wrapper **retData, char **retJson)
{
	// this function is used to parse a nc connection received by mongoose
	// if the connection contains a valid json it can be parsed by our helperfunction and put in
	// the right structurs
	// else
	// log the errors
	// and return false
    struct data_wrapper *data = NULL;
    char *json = NULL; // used to log
    char inbuf[512]; // TODO determine size
	ssize_t sz = crlf_recv (sock, inbuf, sizeof(inbuf));

    if (sz > 0) {
        json = CALLOC (sz + 1, sizeof (char));
        strncpy (json, inbuf, sz * sizeof (char));
		json[sz] = '\0';
        data = convert_string_to_datastruct (json); // parse a datastruct from the message received
    } else {
        return false;
    }

    if (data == NULL && json != NULL ){
        // the json was invalid
        // and logged to errlog
        log_err (json);
        return false;
    }

    // at this point the inbuf contained valid data, put them in the structures and return
    *retJson = json; *retData = data;
    return true; // returns a struct containing both data_wrapper struct and json char
    // the bool is used to check wheter they were allocated successfully
}

void
free_data_wrapper (struct data_wrapper *data)
{
	if (data->msg != NULL) {
		FREE (data->msg);
	}
	if (data->date != NULL) {
		FREE (data->date);
	}
	if (data->id != NULL) {
		FREE (data->id);
	}
	FREE (data);
}

void
announce_exit (struct data_wrapper *data, int sock)
{
	// reply to client saying that you ACK the exit request and
	// the server is exiting
	data->cmd = END;
	FREE (data->msg);
	data->msg = STRDUP ("");
	char *jOk = convert_datastruct_to_char (data);
	/*log_info (msg);*/
	crlf_send(sock, jOk, strlen(jOk));
	FREE (jOk);
}


coroutine static void
send_routine(const int clientSock, struct data_wrapper *data)
{
	// sends the message from the client to tor (other peer)
	// also sends an ack to the client if the send was successful
	char *id;
	
	// set the data wrapper for sending
	data->cmd = RECV;
	id = STRDUP (data->id);
	strcpy (data->id, HOSTNAME);

	char *msg = convert_datastruct_to_char (data);
	int ret = send_over_tor (id, data->portno, msg);

	if (ret != 0) {
		// this informs the client that an error has happened
		// substitute cmd with ERR and msg with RFC error
		data->cmd = ERR;
		FREE(data->msg);
		data->msg = get_tor_error(); // gets the global variable that corresponds to the error (clientSocks_helper.c)
		char *jError = convert_datastruct_to_char (data);
		log_err (jError);
		crlf_send(clientSock, jError, strlen(jError));
		FREE(jError);
	} else {
		data->cmd = END;
		FREE(data->msg);
		data->msg = STRDUP (""); // is just an ACK, message can be empty
		char *jOk = convert_datastruct_to_char (data);
		log_info (jOk);
		crlf_send (clientSock, jOk, strlen(jOk));
		FREE (jOk);
	}
	FREE (msg);
	FREE (id);
	/*free_data_wrapper (data);*/
}

void
relay_msg (const int clientSock, struct data_wrapper *data )
{
	if(data == NULL){
		exit_error("Invalid data structure.");
	}
	go(send_routine(clientSock, data));
	return;
}

void
store_msg (struct data_wrapper *data)
{
	// server received a message
	// store it in hash table
	if (!peer_exist (data->id)) {
		// guess what, peer doesn't exist
		insert_peer (data->id);
		// insert in hash tables only peer that sent RECV, not other cmd
	}
	insert_new_message (data->id, data->msg, data->cmd);
}

void
client_update (struct data_wrapper *data, int sock)
{
	// the client asks for unread messages from data->id peer
	// get the OLDEST, send as a json
	// this is supposed to be executed periodically
	// by the client
	FREE(data->id);
	data->id = STRDUP (data->msg);
	data->id[strlen(data->id)] = '\0';
	// if no msg, get_unread_message should return NULL
	struct message *msg = NULL;
	if((msg = get_unread_message(data->msg)) != NULL){
		// now we convert the message in a json and send it
		FREE (data->msg);
		FREE (data->date);
		// store the field of struct message in datawrapper
		data->msg = STRDUP (msg->content); //content is the message received by the server from another peer
		data->cmd = msg->cmd;
		data->date = STRDUP (msg->date);
	} else {
		data->cmd = END;
	}
	char *unreadMsg = convert_datastruct_to_char(data);
	crlf_send (sock, unreadMsg, strlen(unreadMsg));
	FREE (unreadMsg);
	if(msg){
		FREE (msg->date); FREE (msg->content); FREE (msg);	
	}
}

void
send_hostname_to_client(struct data_wrapper *data, int sock, char *hostname)
{
	// the hostname is sent as a json (similarly to the peer list function below)
	// the hostname is in the data->msg field, this is an explicit request from the client
	//
	FREE(data->msg);
	data->msg = STRDUP(hostname);
	if(data->msg == NULL){
		data->msg = STRDUP("");
		// should be a connection error here, but better be safe
	}

	char *response = convert_datastruct_to_char (data);
	// if iface is not null the client is waiting for response
	int cr = crlf_send (sock, response, strlen (response));
	FREE (response);
}

void
send_peer_list_to_client (struct data_wrapper *data, int sock)
{
	// the client asked to receive the list of all the peers that send the server a message (not read yet)
	// send the list as a parsed json, with peer field comma divided
	// the char containing the list of peers commadivided is then put into a json 
	// just store it in data.msg
	FREE (data->msg);
	data->msg = get_peer_list ();
	if (data->msg == NULL) {
		data->msg = STRDUP ("");
		// needed if no peers messaged us
	}
	char *response = convert_datastruct_to_char (data);
	// if iface is not null the client is waiting for response
	crlf_send (sock, response, strlen (response));
	FREE (response);
}
