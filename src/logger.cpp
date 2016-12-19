#define LOGURU_IMPLEMENTATION 1
#include "../include/loguru.hpp"
//loguru::init (argc, argv);
//#define LOGURU_CATCH_SIGABRT 1
//#define LOGURU_UNSAFE_SIGNAL_HANDLER 1

extern "C" void
log_info (char *json)
{
	LOG_F (INFO, json);
}

extern "C" void
log_err (char *err)
{
	LOG_F (ERROR, err);
}

extern "C" void
log_init (char *name, char *verbosity)
{
	if (strcmp (verbosity, "INFO") == 0) {
		loguru::add_file (name, loguru::Append, loguru::Verbosity_INFO);
	} else if (strcmp (verbosity, "ERROR") == 0) {
		loguru::add_file (name, loguru::Append, loguru::Verbosity_ERROR);
	}
	// other cases
}
