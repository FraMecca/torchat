#pragma once
#define MAXCONN 1024
#define MAXEVENTS MAXCONN
#define EPOLLFLAGS EPOLLIN

int bind_and_listen(const int portno,int n);
int fd_unblock(int fd);

void destroy_proxy_connection ();
void connect_to_tor (const char *host, const int port);
int fd_connect (char *address, int port);
