#include "json.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>
using json = nlohmann::json;


enum command {
	SEND,
	RECV
};
struct data_wrapper {
	// contains the content of the message passed
	enum command cmd;
	char ip[20];
	int portno;
	char *msg;
	// message will be like:
	// IP PORTNO CMD CONTENT...
	// space is the delimiter
};

int 
main (void)
{
	json j;

	j["cmd"] = "RECV";
	j["ip"] = "127.0.0.1";
	j["portno"] = 80;
	j["msg"] = "this is a sample json msg";
	std::cout <<j << std::endl;
	
	std::string s = j.dump ();
	char *st = strdup (s.c_str ());
	printf ("%s IIII\n", st);

	auto j3 = json::parse (st);
	std::cout <<j3 << std::endl;



	return 0;
}
