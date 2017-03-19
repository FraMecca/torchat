#pragma once

bool
send_message_to_socket (struct message *msgStruct, char *peerId);

bool
send_over_tor (const char *domain, const int portno, const char *buf, const int torPort);

int
handshake_with_tor (const char *domain, const int portno, const int torPort);
