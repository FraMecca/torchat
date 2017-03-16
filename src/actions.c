#include "include/mongoose.h"  // Include Mongoose API definitions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/mem.h"
#include "lib/datastructs.h"
#include "lib/socks_helper.h"
#include "lib/util.h"
#include <pthread.h>
#include "actions.h"
extern struct data_wrapper convert_string_to_datastruct (const char *jsonCh);  // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper *data);  // from json.cpp
extern void log_info (char *json); // from logger.cpp
extern void log_err (char *json); // from logger.cpp
extern char * HOSTNAME;
// in this file there are the various functions used by main::event_routine
// related to the various commands
//
//
struct data_conn { 
	// used to wrap dataW and nc for pthread
	struct data_wrapper *dataw;
	struct mg_connection *conn;
};


void
free_data_wrapper (struct data_wrapper *data)
{
	if (data->msg != NULL) {
		FREE (data->msg);
	}
	if (data->date != NULL) {
		FREE (data->date);
	}
	FREE (data);
}

void
announce_exit (struct data_wrapper *data, struct mg_connection *nc)
{
	// reply to client saying that you ACK the exit request and
	// the server is exiting
	data->cmd = END;
	FREE (data->msg);
	data->msg = STRDUP ("");
	char *jOk = convert_datastruct_to_char (data);
	/*log_info (msg);*/
	MONGOOSE_SEND(nc, jOk, strlen(jOk));
	FREE (jOk);
}

static char *
explain_sock_error (const char e)
{
	/*
	 * in case of error the message returned to the client has in the msg field
	 * an explanation of the error, see http://www.ietf.org/rfc/rfc1928.txt
	 */
	switch (e) {
		case  9 :
			// not in RFC,
			// used when handshake fails, so probably TOR has not been started
			return STRDUP ("Could not send message. Is TOR running?");
		case 1 :
			return STRDUP ("general SOCKS server failure");
		case 2 :
			return STRDUP ("connection not allowed by ruleset");
		case 3 :
			return STRDUP ("Network unreachable");
		case 4 :
			return STRDUP ("Host unreachable");
		case 5 :
			return STRDUP ("Connection refused");
		case 6 : 
			return STRDUP ("TTL expired");
		case 7 :
			return STRDUP ("Command not supported");
		case 8 :
			return STRDUP ("Address type not supported");
		default :
			return STRDUP ("TOR couldn't send the message"); // shouldn't go here
	}
}

static void *
send_routine(void *d)
{
	/*pthread_detach(pthread_self()); // needed to avoid memory leaks*/
	// there is no need to call pthread_join, but thread resources need to be terminated
	//
	char id[30];
	struct data_conn *dc = (struct data_conn*) d;
	struct data_wrapper *data = dc->dataw;
	struct mg_connection *nc = dc->conn;


	// needed for file upload
	if (data->cmd == FILEALLOC){
		data->cmd = FILEUP;
		strcpy (id, data->msg); // save dest address
		data->portno = 80;
	} else if (data->cmd != FILEPORT){
			data->cmd = RECV;
	}
	strcpy (id, data->id); // save dest address
	strcpy (data->id, HOSTNAME);

	char *msg = convert_datastruct_to_char (data);
	char ret = send_over_tor (id, data->portno, msg, 9250);

	if (ret != 0) {
		// this informs the client that an error has happened
		// substitute cmd with ERR and msg with RFC error
		data->cmd = ERR;
		data->msg = explain_sock_error (ret);
		char *jError = convert_datastruct_to_char (data);
		log_err (jError);
		MONGOOSE_SEND(nc, jError, strlen(jError));
		FREE(jError);
	} else if (data->cmd != FILEPORT && data->cmd != FILEUP){ // fileport and fileup do not require jOk to be sent
		data->cmd = END;
		data->msg = STRDUP (""); // is just an ACK, message can be empty
		char *jOk = convert_datastruct_to_char (data);
		log_info (jOk);
		MONGOOSE_SEND (nc, jOk, strlen(jOk));
		FREE (jOk);
	}
	FREE (msg);
	free_data_wrapper (data);
	pthread_exit(0); // implicit
}

void
relay_msg (struct data_wrapper *data, struct mg_connection *nc)
{
	struct data_conn *dc = calloc(1, sizeof(struct data_conn));

	/*struct mg_connection *newnc = MALLOC (sizeof (struct mg_connection));*/
	/*memcpy (newnc, nc, sizeof (struct mg_connection));*/
	/*newnc->iface = MALLOC (sizeof (struct mg_iface));*/
	/*memcpy (newnc->iface, nc->iface, sizeof (struct mg_iface));*/
	/*newnc->iface->vtable = MALLOC (sizeof (struct mg_iface_vtable));*/
	/*memcpy (newnc->iface->vtable, nc->iface->vtable, sizeof (struct mg_iface_vtable));*/
	dc->conn = nc;
	dc->dataw = data;

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
	pthread_create(&t, &attr, &send_routine,(void*) dc);
	/*free(dataw);*/
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
	insert_new_message (data->id, data->msg);
}

void
client_update (struct data_wrapper *data, struct mg_connection *nc)
{
	// the client asks for unread messages from data->id peer
	// get the OLDEST, send as a json
	// this is supposed to be executed periodically
	// by the client
	strncpy(data->id, data->msg,29*sizeof(char));
	data->id[strlen(data->id)] = '\0';
	// if no msg, get_unread_message should return NULL
	char *msg;
	if((msg = get_unread_message(data->msg)) != NULL){
		// now we convert the message in a json and send it
		FREE (data->msg);
		data->msg = msg;
	} else {
		data->cmd = END;
	}
	char *unreadMsg = convert_datastruct_to_char(data);
	MONGOOSE_SEND (nc, unreadMsg, strlen(unreadMsg));
	FREE (unreadMsg);
}

void
send_fileport_to_client(struct data_wrapper *data, struct mg_connection *nc)
{
	// the port is sent as a json 
	// the port is in the data->msg field

	char *response = convert_datastruct_to_char (data);
	// if iface is not null the client is waiting for response
	MONGOOSE_SEND (nc, response, strlen (response));
	FREE (response);

}
void
send_hostname_to_client(struct data_wrapper *data, struct mg_connection *nc, char *hostname)
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
	MONGOOSE_SEND (nc, response, strlen (response));
	FREE (response);
}

void
send_peer_list_to_client (struct data_wrapper *data, struct mg_connection *nc)
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
	MONGOOSE_SEND (nc, response, strlen (response));
	FREE (response);
}
