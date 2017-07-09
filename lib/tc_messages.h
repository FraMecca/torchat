#define MSIZEMAX  (16 * 16 * 16 * 16) * 2 / 3

int tc_mrecv  (int fd, unsigned char *buf);
int tc_mclose (int fd);
int tc_msend  (int fd, unsigned char *buf,size_t len);
int tc_message_attach(int fd);
