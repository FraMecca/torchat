#include <stdio.h>
#include <stddef.h>
#include "../include/argparse.h"
#include "../include/mem.h"
#include "../lib/settings.h"
#include "../lib/parseconfig.h"

static struct settings_t settings;

void * 
get_setting(int p)
{
	void *ptr = ((char *) &settings + p);
	return ptr;
}

void
destroy_settings ()
{
	FREE (settings.host);
	FREE (settings.logInfo);
	FREE (settings.logDebug);
	FREE (settings.logError);
	FREE (settings.interface);
}

int main
/*read_torchat_configs (char *filename)*/
(int argc, char **argv)
{
	char **fileArgv = NULL;
	int fileArgc = parse_config (argv[1], &fileArgv);
	if (fileArgc == -1) {
		perror ("sostituisci con exti_error");
		exit(1);
	}

	// gonna use argparse
	struct argparse_option options[] = {
		OPT_GROUP("Config file:"),
		OPT_INTEGER(0, "maxconnections", &settings.maxconnections, ""),
		OPT_INTEGER(0, "maxevents", &settings.maxevents, ""),
		OPT_LONG(0, "epolltimeout", &settings.epollTimeout, ""),
        OPT_STRING(0, "host", &settings.host, ""),
		OPT_INTEGER(0, "torport", &settings.torPort, ""),
		OPT_INTEGER(0, "daemonport", &settings.daemonPort, ""),
		OPT_STRING(0, "loginfo", &settings.logInfo, ""),
		OPT_STRING(0, "logdebug", &settings.logDebug, ""),
		OPT_STRING(0, "logerror", &settings.logError, ""),
		OPT_INTEGER(0, "sockbufsize", &settings.sockBufSize, ""),
		OPT_STRING(0, "interface", &settings.interface, ""),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    argparse_parse(&argparse, 2*fileArgc, fileArgv);
	return 0;
}
