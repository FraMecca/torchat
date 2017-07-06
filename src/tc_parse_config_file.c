/*
 * This file contains the functions to parse the config file
 * It uses libConfuse
 */

#include <stdio.h>
#include "include/libConfuse/src/confuse.h"
#include "include/except.h"

extern Except_T ConfigFileNotFound; 
extern Except_T ConfigFileParseError;

void
parse_config_file (const char *filename, long int *port, long int *torPort)
{
	/* Although the macro used to specify an integer option is called
	 * CFG_SIMPLE_INT(), it actually expects a long int. On a 64 bit system
	 * where ints are 32 bit and longs 64 bit (such as the x86-64 or amd64
	 * architectures), you will get weird effects if you use an int here.
	 *
	 * If you use the regular (non-"simple") options, ie CFG_INT() and use
	 * cfg_getint(), this is not a problem as the data types are implicitly
	 * cast.
	 */
	cfg_opt_t opts[] = {
		/*CFG_SIMPLE_BOOL("verbose", &verbose),*/
		/*CFG_SIMPLE_STR("user", &username),*/
		/*CFG_SIMPLE_FLOAT("delay", &delay),*/
		CFG_SIMPLE_INT("port", port), // read comment above
		CFG_SIMPLE_INT("torport", torPort),
		CFG_END()
	};
	cfg_t *cfg;
	cfg = cfg_init(opts, 0);
	int error = cfg_parse(cfg, filename);
	cfg_free (cfg);

	if (error == CFG_SUCCESS) {
		// no error parsing the config file
		return;
	} else if (error == CFG_FILE_ERROR) {
		RAISE (ConfigFileNotFound);
	} else if (error == CFG_PARSE_ERROR) {
		RAISE (ConfigFileParseError);
	}
}
