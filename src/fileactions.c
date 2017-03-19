#include "lib/file_upload.h"
#include "lib/datastructs.h"
#include "include/mem.h"
#include "lib/socks_helper.h"
#include "include/base64.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h> // perror

extern char * convert_filestruct_to_char (const struct fileAddr *file, const enum command cmd, const char *buf);
extern void log_err (char *json); // from logger.cpp

static char *
get_filename (char *data_fname)
{
	// TODO : fare a modo
	// return basename of the file prefixed by the upload directory designated by the config file
	// see manpages on the allocations needed by basename
	/*char *tmp = STRDUP (data_fname);*/
	/*char *fname = basename (tmp); // basename is needed because we want only the relative path*/
	/*FREE (tmp);*/
	/*char *buf = calloc (sizeof (char), strlen (uploadDir) + strlen (fname) + 1);*/
	/*strcat (buf, uploadDir); strcat (buf, fname); // uploadDir + fname + \n*/
	/*return buf;*/
	return STRDUP (data_fname);
}

void
create_file (struct data_wrapper *data)
{
	FILE *fp = fopen (get_filename (data->fname), "wb");
	// content is reset
	fclose (fp);
	return;
}

static unsigned char *
encode_base64 (const unsigned char *bstr, size_t *outLen)
{
	// decode using http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
	unsigned char *st = base64_encode ((const unsigned char *) bstr, 296, outLen);
	return st;
} 

static unsigned char *
decode_base64 (const char *bstr, size_t *outLen)
{
	// decode using http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
	unsigned char *st = base64_decode ((const unsigned char *) bstr, strlen (bstr), outLen);
	return st;
}

void
write_to_file (struct data_wrapper *data)
{
	// write in append mode the data received from the peer
	// data is base64
	char *fname = get_filename (data->fname);
	FILE *fp;
	fp = fopen (fname, "ab");
	FREE (fname);

	// get the data from the encoded base64
	// that is stored it json['msg']
	size_t outLen = 0;
	unsigned char *fdata = decode_base64 (data->msg, &outLen);
	if (outLen > 0) {
		fwrite (fdata, outLen, sizeof (unsigned char), fp); 
	}
	// TODO: Error checking
	FREE (fdata);
	fclose (fp);
}

void *
send_file_routine (void *fI)
{
	// after the connection to the port is established
	// simply send the file
	struct fileAddr *file = (struct fileAddr *) fI;

	// the filename is the name you would give to the file sent
	// the filepath is the absolute path to the file
	//
	// PART 1, FILEBEGIN, grab the file and send metadata
	{
		FILE *fd = fopen (file->path, "rb");
		if (fd){
			// check that file exists
			fclose (fd);
		} else {
			char err[20 + strlen(file->path)];
			sprintf(err, "File %s not found.", file->path);
			log_err(err);
			pthread_exit (NULL);
		}
		char *json = convert_filestruct_to_char (file, FILEBEGIN, NULL);
		send_over_tor (file->host, atoi (file->port), json, 9250); // slow
	}

	// PART 2, FILEDATA, read and send the file
	{
		FILE *fp = fopen (file->path, "rb");
		// fread returns zero even if it read less than requested size
		// for this reason reset buf at every iteration
		// and stop if both fread == 0 and buf not changed
		unsigned char buf[296] = "";
		while (fread (buf, 296, sizeof (unsigned char), fp) || buf[0] != '\0') {
			char *jbuf = (char *) encode_base64 (buf, NULL); // here the encoded binary data
			char *json = convert_filestruct_to_char (file, FILEDATA, jbuf);
			send_over_tor (file->host, atoi (file->port), json, 9250); // slow
			buf[0] = '\0';
		}
		fclose (fp);
	}

	// PART 3, FILEEND, ACK end of file
	{
		char *json = convert_filestruct_to_char (file, FILEEND, NULL);
		send_over_tor (file->host, atoi (file->port), json, 9250); // slow
	}
	pthread_exit (NULL);
}
