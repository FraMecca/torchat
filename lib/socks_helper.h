#pragma once

char *get_tor_error ();
bool send_over_tor (int torSock, const char *domain, const int portno, const char *buf);
int handshake_with_tor (const char *domain, const int portno, const int torPort);
int
open_socket_to_domain(int sock, const char *domain, const int portno);
