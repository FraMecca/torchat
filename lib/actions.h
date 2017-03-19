#pragma once

char *read_tor_hostname (void);
void announce_exit (struct data_wrapper *data, struct mg_connection *nc);
void relay_msg (struct data_wrapper *data, struct mg_connection *nc);
void store_msg(struct data_wrapper *data);
void client_update(struct data_wrapper *data,struct mg_connection *nc);
void send_fileport_to_client(struct data_wrapper *data, struct mg_connection *nc);
void send_peer_list_to_client(struct data_wrapper *data,struct mg_connection *nc);
void send_hostname_to_client(struct data_wrapper *data, struct mg_connection *nc, char *hostname);
//void ev_handler(struct mg_connection *nc, int ev, void *ev_data);
bool parse_mongoose_connection (struct mg_connection *nc, struct data_wrapper **retData, char **retJson);

#define MONGOOSE_SEND(nc, json, len) if (nc->iface && nc->iface->vtable) mg_send (nc, json, len)


