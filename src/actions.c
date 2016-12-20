#include "../include/mongoose.h"  // Include Mongoose API definitions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../lib/datastructs.h"
#include "../lib/socks_helper.h"
#include "../lib/util.h"
extern struct data_wrapper convert_string_to_datastruct (const char *jsonCh);  // from json.cpp
extern char * convert_datastruct_to_char (const struct data_wrapper *data);  // from json.cpp
extern char * HOSTNAME;
// in this file there are the various functions used by main::event_routine
// related to the various commands
//

void *
send_routine(void *d)
{
	char id[30];
	struct data_wrapper *data = (struct data_wrapper*)d;

	strcpy (id, data->id); // save dest address
	strcpy (data->id, HOSTNAME);
	data->cmd = RECV;
	free (data->date);
	data->date = get_short_date();

	char *msg = convert_datastruct_to_char (data);
	bool ret = send_over_tor (id, data->portno, msg, 9250);

	if (!ret) {
		exit_error ("send_over_tor");
	}
	free (msg);
	pthread_exit(NULL);
}

// relay client msg to the another peer on the tor network
void
relay_msg (struct data_wrapper *data)
{
	pthread_t t;

	pthread_create(&t, NULL, &send_routine,(void*) data);
	/*pthread_join(t, NULL);*/
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
		free (data->msg);
		data->msg = msg;
	} else {
		data->cmd = END;
	}
	char *unreadMsg = convert_datastruct_to_char(data);
	mg_send (nc, unreadMsg, strlen(unreadMsg));
	free (unreadMsg);
}

void
send_peer_list_to_client (struct data_wrapper *data, struct mg_connection *nc)
{
	// the client asked to receive the list of all the peers that send the server a message (not read yet)
	// send the list as a parsed json, with peer field comma divided
	// the char containing the list of peers commadivided is then put into a json 
	// just store it in data.msg
	free (data->msg);
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
	free (response);
}
