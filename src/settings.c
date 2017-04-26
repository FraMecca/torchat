/*#include "lib/settings.h"*/
#include <stdio.h>
#include <string.h>


#define MAXCONNECTIONS 0
#define MAXEVENTS 1

struct settings_t {
	int maxconnections;
	int maxevents;
	long int epollTimeout;
	char *host;
	int torPort;
	int daemonPort;
	char *logInfo;
	char *logDebug;
	char *logError;
	size_t sockBufSize;
	char *interface;
};

static struct settings_t settings;

int*
get_setting(int p)
{
	return (&settings.maxconnections+p);
}

int
main(void){
	settings.maxconnections = 1;
	settings.maxevents = 2;
	printf("%d %d\n", *get_setting(MAXCONNECTIONS), *get_setting(MAXEVENTS));
	return 0;
}
