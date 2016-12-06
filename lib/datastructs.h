#pragma once



enum command {
	SEND,
	RECV,
	EXIT
};
struct data_wrapper {
	// contains the content of the message passed
	enum command cmd;
	char id[30];
	int portno;
	char *msg;
	// message will be like:
	// IP PORTNO CMD CONTENT...
	// space is the delimiter
};
