#pragma once
#include "lib/tc_json.h"
#define MSIZEMAX  3276 - 1

int tc_mrecv  (int fd);
int tc_mclose (int fd);
int tc_msend  (int fd, JSON *j);
int tc_message_attach(int fd);
void tc_messages_destroy ();
