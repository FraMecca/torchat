#define LOGURU_IMPLEMENTATION 1
#include "include/loguru.hpp"
//loguru::init (argc, argv);
//#define LOGURU_CATCH_SIGABRT 1
//#define LOGURU_UNSAFE_SIGNAL_HANDLER 1
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
	LOG_F (INFO, json);
}

extern "C" void
log_err (const char *err)
{
	LOG_F (ERROR, err);
}

extern "C" void
log_init (const char *name, const char *verbosity)
{
	// initialize a log 
	if (strcmp (verbosity, "INFO") == 0) {
		infoLog = strdup (name);
		loguru::add_file (name, loguru::Append, loguru::Verbosity_INFO);
	} else if (strcmp (verbosity, "ERROR") == 0) {
		errLog = strdup (name);
		loguru::add_file (name, loguru::Append, loguru::Verbosity_ERROR);
	} else if (strcmp (verbosity, "DEBUG") == 0) {
		debLog = strdup (name);
		loguru::add_file (name, loguru::Append, loguru::Verbosity_MAX);
	} // other cases
}

extern "C" void
log_clear_datastructs ()
{
	loguru::remove_all_callbacks();
	if (infoLog != NULL) {
		free (infoLog);
	}
	if (errLog != NULL) {
		free (errLog);
	}
	if (debLog != NULL) {
		free (debLog);
	}
	free(sem);
}
