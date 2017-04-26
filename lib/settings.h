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

#define MAXCONNECTIONS offsetof (struct settings_t, maxconnections)
#define MAXEVENTS offsetof (struct settings_t, maxevents)
#define EPOLLTIMEOUT offsetof (struct settings_t, epollTimeout)
#define HOST offsetof (struct settings_t, host)
#define TORPORT offsetof (struct settings_t, torPort)
#define DAEMONPORT offsetof (struct settings_t, daemonPort)
#define LOGINFO offsetof (struct settings_t, logInfo)
#define LOGDEBUG offsetof (struct settings_t, logDebug)
#define LOGERROR offsetof (struct settings_t, logError)
#define INADDR offsetof (struct settings_t, interface) // servAddr.sin_addr.s_addr = INADDR_ANY; 
