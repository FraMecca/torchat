#pragma once

char *read_tor_hostname (void);
void announce_exit (struct data_wrapper *data, int sock);
void relay_msg (struct data_wrapper *data, int sock, int torSock, int64_t deadline);
void store_msg(struct data_wrapper *data);
void client_update(struct data_wrapper *data,int sock, int64_t deadline);
void send_fileport_to_client(struct data_wrapper *data, int sock, int64_t deadline);
void send_peer_list_to_client(struct data_wrapper *data,int sock, int64_t deadline);
void send_hostname_to_client(struct data_wrapper *data, int sock, char *hostname, int64_t deadline);
//void ev_handler(int sock, int ev, void *ev_data);
bool parse_connection (int sock, struct data_wrapper **retData, char **retJson, int64_t deadline);

#define SOCK_SEND(sock, json, len, deadline) msend(sock, json, len, deadline)
