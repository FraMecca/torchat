void initialize_proxy_connection(const char *host,const int port);
void terminate_connection_with_domain(const int sock);
void destroy_proxy_connection();
char *get_tor_error();
bool set_socket_timeout (const int sockfd);
int open_socket_to_domain(const char *domain,const int portno);
ssize_t send_over_tor(const char *domain,const int port,char *buf);
ssize_t crlf_send(int sock,const char *buf,size_t len);
ssize_t crlf_recv(const int sock,char *retBuf,const size_t maxSz);
int
bind_and_listen (const int portno);
