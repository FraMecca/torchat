#include "include/except.h"
Except_T ConfigFileNotFound = {"Config file not found, use --config-file option"};
Except_T ConfigFileParseError = {"Config file parse error"};
void parse_config_file (const char *filename, int *port, int *torPort, char **torDomain);
