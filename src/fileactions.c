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

#define BUFSIZE 512

static char *
get_filename (char *data_fname)
{
	// return basename of the file prefixed by the upload directory designated by the config file
	// upload dir is harcoded until a config file is provided
	// TODO config file
	char uploadDir[15];
	strncpy(uploadDir, "Files_Received/", 14);
	char *buf = calloc (sizeof (char), strlen (uploadDir) + strlen (data_fname) + 1);
	strcat (buf, uploadDir); strcat (buf, data_fname); // uploadDir + fname + \n
	return STRDUP (buf);
}

void
create_file (struct data_wrapper *data)
{
	char *path = get_filename(data->fname);
	FILE *fp = fopen (path, "wb");
	// content is reset
	free(path);
	fclose (fp);
	return;
}

static unsigned char *
encode_base64 (const unsigned char *bstr, size_t *outLen)
{
	// decode using http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
	unsigned char *st = base64_encode ((const unsigned char *) bstr, BUFSIZE, outLen);
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

coroutine void
send_file_routine (const int sock, struct fileAddr *file)
{
	// after the connection to the port is established
	// simply send the file
	// the filename is the name you would give to the file sent
	// the filepath is the absolute path to the file

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
			exit_error("File not found.");
		}
		char *json = convert_filestruct_to_char (file, FILEBEGIN, NULL);
		if (send_over_tor (sock, file->host, atoi (file->port), json) < 0){
			exit_error("Cannot send file metadata, send_file_routine");
		}
	}

	// PART 2, FILEDATA, read and send the file
	{
		FILE *fp = fopen (file->path, "rb");
		// fread returns zero even if it read less than requested size
		// for this reason reset buf at every iteration
		// and stop if both fread == 0 and buf not changed
		unsigned char buf[BUFSIZE] = "";
		while (fread (buf, BUFSIZE, sizeof (unsigned char), fp) || buf[0] != '\0') {
			// here the encoded binary data
			char *jbuf = (char *) encode_base64 (buf, NULL); 
			char *json = convert_filestruct_to_char (file, FILEDATA, jbuf);
			// and send
    		if (send_over_tor(sock, file->host, atoi(file->port), json) < 0) {
    			exit_error ("Can't send over socket, send_file_routine");
    		}
			// reset the buffer
			buf[0] = '\0';
		}
		fclose (fp);
		close (sock);
	}

	// PART 3, FILEEND, ACK end of file 
	// TODO decide if this is actually useful
	{
		char *json = convert_filestruct_to_char (file, FILEEND, NULL);
		send_over_tor (sock, file->host, atoi (file->port), json);
	}
	free_file(file);
}
