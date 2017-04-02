#define LOGURU_IMPLEMENTATION 1
#include "include/loguru.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

static pthread_mutex_t *sem = NULL; // mutex to initialize when the log files are used outside of loguru
// eg: when parsing the logs

static char *infoLog = NULL; // store name of the infoLog
static char *errLog = NULL; // same, err
static char *debLog = NULL; // same, debug

/*
 * loguru is already thread safe,
 * so no need to use mutex for luguru actions
 */

extern "C" void
log_info (const char *json)
{
	return;
}

extern "C" void
log_err (const char *err)
{
	return;
}

extern "C" void
log_init (const char *name, const char *verbosity)
{
	// initialize a log 
	return ;
}

extern "C" void
log_clear_datastructs ()
{
	return;
}
