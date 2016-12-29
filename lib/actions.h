#pragma once
char *read_tor_hostname (void);
void relay_msg(struct data_wrapper *data);
void store_msg(struct data_wrapper *data);
void client_update(struct data_wrapper *data,struct mg_connection *nc);
void send_peer_list_to_client(struct data_wrapper *data,struct mg_connection *nc);
void send_hostname_to_client(struct data_wrapper *data, struct mg_connection *nc, char *hostname);
