/* This file was automatically generated.  Do not edit! */
size_t crlf_recv(const int sock,char *retBuf,const size_t maxSz);
int crlf_send(int sock,const char *buf,size_t len,int f);
int open_socket_to_domain(const char *domain,const int portno);
int handshake_with_tor(const char *domain,const int port);
char *get_tor_error();
void destroy_proxy_connection();
void terminate_connection_with_domain(const int sock);
void initialize_proxy_connection();
