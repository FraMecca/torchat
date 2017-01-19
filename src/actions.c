#include "include/mongoose.h"  // Include Mongoose API definitions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/mem.h"
#include "lib/datastructs.h"
#include "lib/socks_helper.h"
#include "lib/util.h"
extern struct data_wrapper convert_string_to_datastruct (const char *jsonCh);  // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper *data);  // from json.cpp
extern char * generate_error_json (const struct data_wrapper *data, char *error);
extern void log_info (char *json); // from logger.cpp
extern void log_err (char *json); // from logger.cpp
extern char * HOSTNAME;
// in this file there are the various functions used by main::event_routine
// related to the various commands
//

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
	data->msg = strdup ("");
	char *jOk = convert_datastruct_to_char (data);
	/*log_info (msg);*/
	mg_send(nc, jOk, strlen(jOk));
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
			return strdup ("Could not send message. Is TOR running?");
		case '1' :
			return strdup ("general SOCKS server failure");
		case '2' :
			return strdup ("connection not allowed by ruleset");
		case '3' :
			return strdup ("Network unreachable");
		case '4' :
			return strdup ("Host unreachable");
		case '5' :
			return strdup ("Connection refused");
		case '6' : 
			return strdup ("TTL expired");
		case '7' :
			return strdup ("Command not supported");
		case '8' :
			return strdup ("Address type not supported");
		default :
			return strdup ("TOR couldn't send the message"); // shouldn't go here
	}
}

// relay client msg to the another peer on the tor network
void
relay_msg (struct data_wrapper *data, struct mg_connection *nc)
{
	char id[30];
	strcpy (id, data->id); // save dest address
	strcpy (data->id, HOSTNAME);
	data->cmd = RECV;

	char *msg = convert_datastruct_to_char (data);
	char ret = send_over_tor (id, data->portno, msg, 9250);
	FREE (msg);
	FREE (data->msg); // substitude below

	if (ret != 0) {
		// this informs the client that an error has happened
		// substitute cmd with ERR and msg with RFC error
		data->cmd = ERR;
		data->msg = explain_sock_error (ret);
		char *jError = convert_datastruct_to_char (data);
		log_err (jError);
		mg_send(nc, jError, strlen(jError));
		FREE(jError);
	} else {
		data->cmd = END;
		data->msg = strdup (""); // is just an ACK, message can be empty
		char *jOk = convert_datastruct_to_char (data);
		/*log_info (jOk);*/
		mg_send(nc, jOk, strlen(jOk));
		FREE (jOk);
	}
	free_data_wrapper (data);
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
	mg_send (nc, unreadMsg, strlen(unreadMsg));
	FREE (unreadMsg);
}

void
send_hostname_to_client(struct data_wrapper *data, struct mg_connection *nc, char*hostname)
{
	// the hostname is sent as a json (similarly to the peer list function below)
	// the hostname is in the data->msg field, this is an explicit request from the client
	//
	FREE(data->msg);
	data->msg = strdup(hostname);
	if(data->msg == NULL){
		data->msg = strdup("");
		// should be a connection error here, but better be safe
	}

	char *response = convert_datastruct_to_char (data);
	if (nc->iface != NULL) {
		// if iface is not null the client is waiting for response
		mg_send (nc, response, strlen (response));
	}
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
		data->msg = strdup ("");
		// needed if no peers messaged us
	}
	char *response = convert_datastruct_to_char (data);
	if (nc->iface != NULL) {
		// if iface is not null the client is waiting for response
		mg_send (nc, response, strlen (response));
	}
	FREE (response);
}
