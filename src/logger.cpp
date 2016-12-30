#define LOGURU_IMPLEMENTATION 1
#include "../include/loguru.hpp"
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
	if (sem != NULL) {
		pthread_mutex_lock (sem);
	}
	// INFO LOG
	LOG_F (INFO, json);
	if (sem != NULL) {
		pthread_mutex_unlock (sem);
	}
}

extern "C" void
log_err (const char *err)
{
	if (sem != NULL) {
		pthread_mutex_lock (sem);
	}
	// log as error
	LOG_F (ERROR, err);
	if (sem != NULL) {
		pthread_mutex_unlock (sem);
	}
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

extern "C" char **
get_history (const char *id, const int n, int *size)
{
	// get last N lines of history
	// if N == 0, get all history
	// return char * array, size is n
	// size stores the total size of return char **
	std::ifstream in (infoLog);
	
	std::vector<std::string> lines;
	if (in.is_open ()) {
		// store the file in a vector
		std::string tmp;
		while (std::getline (in, tmp)) {
			lines.push_back (tmp); // store lines in memory, in the order they are read
		}
		in.close ();
	} else {
		return NULL;
		// could not open file
	}
	// now iterate in reverse if we just need to get only N elements
	std::size_t cnt = 0;
	char **res = (char **) malloc (n * sizeof (char *));
	for (size_t i = 0; i < lines.size (); ++i) {
		std::string tmp = lines[i];
		std::size_t pos = tmp.find ("|");
		if (pos < tmp.length ()) {
			// split the string, | is the delimiter before the message
			res[cnt] = strdup (tmp.substr (pos).c_str ());	 // it is still a list of json, needs to be parsed
			cnt++;
			if ((unsigned) n == cnt) {
				// if n == 0 never enters
				break;
			}
		} // else the string does not contain | and is loguru related, such as backtraces
	}
	*size = cnt;

	return res;
}

