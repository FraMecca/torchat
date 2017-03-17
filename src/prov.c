#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
// these are for file upload functions, the other for file recv
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
	
	int
main ()
{
	// this sends the files through a POST request (http)
	// the form is built by libcurl
	// the filename is the name you would give to the file sent
	// the filepath is the absolute path to the file
	CURL *curl;
	CURLcode res;
	FILE *fd;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;

	// format arguments
	char dest_addr[] = "http://ld74fqvoxpu5yi73.onion:43434" ;

	/*if(!(fd=fopen ("/home/user/a", "rb"))){*/
		/*exit_error("file not found");*/
	/*}*/

	// Fill in the file upload field 
	  curl_formadd(&formpost,
			   &lastptr,
			   CURLFORM_FILENAME, "/home/user/a",
			   CURLFORM_COPYNAME, "/home/user/a",
			   CURLFORM_END);
	// fill in the filename field
	curl_formadd(&formpost,
			   &lastptr,
			   CURLFORM_FILENAME, "/home/user/a",
			   CURLFORM_COPYNAME, "/home/user/a",
			   CURLFORM_COPYCONTENTS, "/home/user/a",
			   CURLFORM_END);

	curl = curl_easy_init();
	if(curl){
		//upload endpoint
		//set SOCKS5 proxy
		curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:9250");
		curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
		curl_easy_setopt(curl, CURLOPT_URL, dest_addr);
		curl_easy_setopt(curl, CURLOPT_PORT, 43434);
		//enable uploading to endpoint
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

		//enable verbose output
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// send request
		if((res = curl_easy_perform(curl)) != CURLE_OK){
			exit( 1);
		}
		curl_easy_cleanup(curl);
	}
	fclose(fd);
}
