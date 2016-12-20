#pragma once
void relay_msg(struct data_wrapper *data,char *hostname);
void store_msg(struct data_wrapper *data);
void client_update(struct data_wrapper *data,struct mg_connection *nc);
void send_peer_list_to_client(struct data_wrapper *data,struct mg_connection *nc);
