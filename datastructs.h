#pragma once



enum command {
	SEND,
	RECV
};
struct data_wrapper {
	// contains the content of the message passed
	enum command cmd;
	char id[20];
	int portno;
	char *msg;
	// message will be like:
	// IP PORTNO CMD CONTENT...
	// space is the delimiter
};
