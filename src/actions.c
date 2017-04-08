#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/mem.h"
#include "lib/datastructs.h"
#include "lib/socks_helper.h"
#include "lib/util.h"
#include <assert.h>
#include "lib/actions.h"
#include "lib/torchatproto.h"
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

int
parse_connection (const int sock, struct data_wrapper **retData, char **retJson, int64_t deadline)
{
	// this function is used to parse a nc connection received by mongoose
	// if the connection contains a valid json it can be parsed by our helperfunction and put in
	// the right structurs
	// else
	// log the errors
	// and return false
    struct data_wrapper *data = NULL;
    char *json = NULL; // used to log
    char inbuf[512];
    memset (inbuf, 0, 512);

	int rc = torchatproto_mrecv (sock, inbuf, 512, deadline);
    if (rc == -1) return -1;  // sz = 0 means connection closed, sz = -1 means error

	size_t sz = strlen (inbuf);
    json = CALLOC (sz + 1, sizeof (char));
    strncpy (json, inbuf, sz * sizeof (char));
	json[sz] = '\0';
    data = convert_string_to_datastruct (json); // parse a datastruct from the message received

	// check data and json validity
    if (data == NULL && json != NULL ){
        // the json was invalid
        // and logged to errlog
        log_err (json);
        // connection was opened but the message was invalid, return 0
        return 0;
    }
    // at this point the inbuf contained valid data, put them in the structures and return
    // the bool is used to check wheter they were allocated successfully
    *retJson = json; *retData = data;
    return rc; // returns a struct containing both data_wrapper struct and json char
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
	FREE (jOk);
}


static void
send_routine(const int clientSock, struct data_wrapper *data, int64_t deadline)
{
	// sends the message from the client to tor (other peer)
	// also sends an ack to the client if the send was successful
	char *id;
	
	// set the data wrapper for sending
	data->cmd = RECV;
	id = STRDUP (data->id);
	strcpy (data->id, HOSTNAME);

	char *msg = convert_datastruct_to_char (data);
	int ret = 0;
	send_over_tor (id, data->portno, msg, deadline, &ret);

	if (ret >= 0) {
		data->cmd = END;
		FREE(data->msg);
		data->msg = STRDUP (""); // is just an ACK, message can be empty
		char *jOk = convert_datastruct_to_char (data);
		log_info (jOk);
		torchatproto_msend (clientSock, jOk, strlen(jOk), deadline);
		FREE (jOk);
	} else {
		// this informs the client that an error has happened
		// substitute cmd with ERR and msg with RFC error
		data->cmd = ERR;
		FREE(data->msg);
		data->msg = get_tor_error(); // gets the global variable that corresponds to the error (clientSocks_helper.c)
		char *jError = convert_datastruct_to_char (data);
		log_err (jError);
		torchatproto_msend (clientSock, jError, strlen(jError), deadline);
		FREE(jError);
	}
	FREE (msg);
	FREE (id);
	/*free_data_wrapper (data);*/
}

void
relay_msg (const int clientSock, struct data_wrapper *data, int64_t deadline)
{
	assert (data != NULL);
	send_routine(clientSock, data, deadline);
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
client_update (struct data_wrapper *data, int sock, int64_t deadline)
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
	torchatproto_msend (sock, unreadMsg, strlen(unreadMsg), deadline);
	FREE (unreadMsg);
	if(msg){
		FREE (msg->date); FREE (msg->content); FREE (msg);	
	}
}

void
send_hostname_to_client(struct data_wrapper *data, int sock, char *hostname, int64_t deadline)
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
	int rc = torchatproto_msend (sock, response, strlen (response), deadline);
	FREE (response);
}

void
send_peer_list_to_client (struct data_wrapper *data, int sock, int64_t deadline)
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
	torchatproto_msend (sock, response, strlen (response), deadline);
	FREE (response);
}
