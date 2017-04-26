#include <stdio.h>
#include <stddef.h>
#include "include/mem.h"
#include "lib/settings.h"


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
