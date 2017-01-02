#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../lib/datastructs.h"
extern struct data_wrapper *convert_string_to_datastruct (const char *jsonCh);  // from json.cpp

static char *
convert_enum (enum command cmd)
{
	if (cmd == RECV) {
		return strdup ("RECV");
	} else if (cmd == SEND) {
		return strdup ("SEND");
	}
}

static void
print_to_file (FILE *fp, struct data_wrapper *data)
{
	char *cmd = convert_enum (data->cmd);
	fprintf (fp, "{\"id\": \"%s\", \"portno\": %d ,\"date\": \"%s\",\"cmd\":\"%s\", \"msg\": \"%s\"}\n",
 		   	data->id, data->portno, data->date, cmd, data->msg);
}

int
main (int argc, char **argv)
{
	FILE *fp = fopen (argv[1], "r");
	char *location = argv[1];
	
	char *buf = NULL;
	size_t n = 0;
	while (getline (&buf, &n, fp) >= 0) { 
		// getline returs -1 if error or EOF
		// and sets errno
		if (strstr (buf, "RECV") != NULL || strstr (buf, "SEND") != NULL) {
			// buf is of any interest
			char *json = strstr (buf, "{");
			// delimiter is included in strstr, in this case |
			// 
			// now allocate a datastruct from the json
			// and append everything to a file with id as filename
			struct data_wrapper *data = convert_string_to_datastruct (json);
			FILE *fc = fopen (data->id, "a");
			print_to_file (fc, data);
			fclose (fc);
		}
		free (buf);
		buf = NULL;
		n = 0;
	}
	free (buf); // read man, buf should be free'd at the end
	fclose (fp);
	
	return 0;
}
