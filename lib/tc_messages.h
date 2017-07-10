#define MSIZEMAX  3276 - 1

int tc_mrecv  (int fd);
int tc_mclose (int fd);
int tc_msend  (int fd, void *buf, size_t len);
int tc_message_attach(int fd);
void tc_messages_destroy ();
