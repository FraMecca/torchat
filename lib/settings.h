#pragma once
#include <stddef.h>

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
	char *interface; // servAddr.sin_addr.s_addr = INADDR_ANY; 
};

void * 
get_setting(int p);

#define MAXCONNECTIONS get_setting (offsetof (struct settings_t, maxconnections))
#define MAXEVENTS get_setting (offsetof (struct settings_t, maxevents))
#define EPOLLTIMEOUT get_setting (offsetof (struct settings_t, epollTimeout))
#define HOSTADDR get_setting (offsetof (struct settings_t, host))
#define TORPORT get_setting (offsetof (struct settings_t, torPort))
#define DAEMONPORT get_setting (offsetof (struct settings_t, daemonPort))
#define LOGINFO get_setting (offsetof (struct settings_t, logInfo))
#define LOGDEBUG get_setting (offsetof (struct settings_t, logDebug))
#define LOGERROR get_setting (offsetof (struct settings_t, logError))
#define INADDR get_setting (offsetof (struct settings_t, interface)) // servAddr.sin_addr.s_addr = INADDR_ANY; 
