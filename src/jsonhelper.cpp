#include "include/json.hpp"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/datastructs.h"
//#include "../lib/util.h"
using json = nlohmann::json;

#ifndef NDEBUG
#include "include/loguru.hpp"
#endif

char *
get_date ()
{
	// copied from util.c
	// get current date
	time_t t = time (NULL);
	struct tm *tm = localtime (&t);
	char date[50];
	strcpy (date, asctime (tm));
	date[strlen (date)-1] = '\0'; // insert linetermination
	return strdup (date);
}

/*
 * json j ={
 * {"cmd" = RECV or SEND},
 * {"portno" = n},
 * {"id" = "id" },
 * {"date" = "date"}, // only when the communication is between client and daemon, not p2p
 * {"msg" = buf}
 * }
 */

static enum  command
convert_to_enum (const std::string cmd)
{
	// convert the json cmd field into the C enum
	if (cmd == "RECV") {
		return RECV;
	} else if (cmd == "SEND") {
		return SEND;
	} else if (cmd == "GET_PEERS") {
		return GET_PEERS;
	} else if (cmd == "EXIT") {
		return EXIT;
	} else if (cmd == "UPDATE") {
		return UPDATE;
	} else if (cmd == "END") {
		return END;
	} else if (cmd == "FILEUP")	{
		return FILEUP;
	} else if (cmd == "FILEALLOC")	{
		return FILEALLOC;
	} else if (cmd == "FILEPORT")	{
		return FILEPORT;
	} else if (cmd == "FILEINFO")	{
		return FILEINFO;
	} else if (cmd == "HOST") {
		return HOST;
	} else /* if (cmd == "ERR") */ {
		// it is the default case
		return ERR;
	}
}

static  std::string
convert_from_enum (const enum command c)
{
	std::string st;
	switch (c) {
		case RECV :
			st = "RECV";
			break;
		case SEND :
			st = "SEND";
			break;
		case EXIT :
			st = "EXIT";
			break;
		case GET_PEERS :
			st = "GET_PEERS";
			break;
		case UPDATE :
			st = "UPDATE";
			break;
		case END :
			st = "END";
			break;
		case HOST :
			st = "HOST";
			break;
		case FILEUP :
			st = "FILEUP";
			break;
		case FILEPORT :
			st = "FILEPORT";
			break;
		case FILEALLOC :
			st = "FILEALLOC";
			break;
		case FILEINFO :
			st = "FILEINFO";
			break;
		case ERR :
			st = "ERR";
			break;
	}
	return st;
}

extern "C" struct data_wrapper *
convert_string_to_datastruct (const char *jsonCh)
{
	// receive a char sent by another peer and translate that into a datawrapper that contains all the informations
	/*
	 * first translate the string to a json
	 * then populate the datastruct 
	 * dumping the json
	 */
	std::string st (jsonCh); // translate char* to std::string
	//if (std::count (st.begin (), st.end (), '}') >= 1) {
		//st.erase (st.find ('}') + 1, std::string::npos); // it seems that mongoose doesn't clean io->buf, so we truncate the string after the first \{
	//} // not used anymore, io->buf is truncated to io->size
#ifndef NDEBUG
	LOG_F (9, jsonCh); 
	std::cout << "json.cpp:" << __LINE__ <<": Received: " << st << std::endl;
#endif

	/*
	 * now parse json
	 * in case of an invalid json
	 * it throws a std::invalid_argument exception
	 * catch that 
	 * and log that to error log
	 */
	json j;
	struct data_wrapper *data;
	try {	
		j = json::parse (st);
	} catch (const std::invalid_argument&) {
		return NULL;
	}
	data = (struct data_wrapper *) calloc (1, sizeof (struct data_wrapper));
	if (data == NULL) {
		std::cerr << "Failed alloc for data_wrapper " << __FILE__ << ":" << __LINE__ << std::endl;
		exit (1);
	}
	memset (data->id, 0, 30);
	std::string jmsg = j["msg"];
	data->msg = strdup (jmsg.c_str());
	std::string jid = j["id"];
	strncpy (data->id, jid.c_str (), strlen (jid.c_str()));
	data->id[strlen (jid.c_str ()) + 1] = '\0';
	data->portno = j["portno"];
	data->cmd = convert_to_enum (j["cmd"]);
	// the json has no date field because communication between servers does not need this info field
	if (j.find("date") != j.end()) {
		// date is specified in the json
		std::string jdate = j["date"];
		data->date = strdup (jdate.c_str ());
	} else {
		data->date = get_date ();
	}

	return data;
}

extern "C" char *
convert_datastruct_to_char (const struct data_wrapper *data)
{
	// does the opposite,
	// takes a data_wrapper and return a char* to be sent over a socket
	/*
	 * first populate a json
	 * then dump json to string
	 */
	json j;
	j["cmd"] = convert_from_enum (data->cmd);
	j["id"] = data->id;
	j["msg"] = data->msg;
	j["portno"] = data->portno;
	if (data->cmd == UPDATE) {
		// the date field is used only to communicate to the client
		// the time of arrival of a message
		// it is not used in server to server communication
		j["date"] = data->date;
	}
	std::string st =  j.dump ();
	return strdup  (st.c_str ());
};
