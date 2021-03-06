/* A tokenizer that can parse the config file 
 * at the moment it is very simple, reads the file and formats the options
 * it supports comments (default char is #)
 * To be included in torchat, by changing main(void) declaration
 * standalone compile with:
 * gcc parse_config.c ../include/mem.c ../include/except.c ../include/ut_assert.c  -o pc -Wall -g
 */
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "../include/mem.h"

// the character that will identify comments
// and the number of options in the config file
#define COMMENT_CHAR '#'
#define SEPARATOR ':'
#define N_OPTS 11
#define SIZEBUF 256 

static void
strip(char *line, char del)
{
	// each commented line / piece of line is ignored by placing a '\0'
	int i;

	if (!line){
		return;
	}
	for(i=0; i<strlen(line); i++){
		if(line[i] == del){
			line[i] = '\0';
		}
	}
}

static void
format_option(char *opt)
{
	// format option is needed since the argparse library we use is built to read command line options
	// therefore, each option needs to be prefixed with "--"
	char tmp[SIZEBUF] = {0};
	strncpy(tmp, "--", 2);
	strncat(tmp, opt, strlen(opt));
	strncpy(opt, tmp, strlen(tmp));
}

static char **
create_opts_vector(int size)
{
	// allocate a vector of size 2*(num options)+1
	// must be NULL terminated
	char **v = MALLOC(size*sizeof(char*));
	return v;
}

static void
fill_opts_vector(char **retOpts, int n, char **opt, char **val, int pos)
{
	// recursively fill the final vector with options and values
	if(pos == n-1){
		retOpts[pos] = NULL;
		return;
	}
	if (pos % 2 == 0){
		retOpts[pos] = STRDUP(opt[pos/2]);
	} else {
		retOpts[pos] = STRDUP(val[(pos-1)/2]);
	}
	fill_opts_vector(retOpts, n, opt, val, ++pos);	
}

static void
destroy_mat(char **mat, int n)
{
	int i;
	for(i=0; i<n; i++){
		FREE(mat[i]);
	}
	FREE(mat);
}

static bool
valid_opt(char *opt)
{
	int l = strlen(opt);
	if(opt[l-1] == ':') return true;
	return false;
}

int
parse_config (char *filename, char ***returnOpts)
{
	assert (*returnOpts == NULL);
	// return -1 on error
	// check errno
	FILE *configFile = fopen(filename, "r");
	if (!configFile){
		/*fprintf(stderr, "Unable to open config file.\n");*/
		// errno set by fopen
		return -1;
	}

	char *lineBuf = NULL, optBuf[SIZEBUF] = {0}, valBuf[SIZEBUF] = {0};
	char **retOpts;
	// opt is used to store the formatted values of option
	// val is for the values
	char **opt = MALLOC(N_OPTS*sizeof(char*));
	char **val = MALLOC(N_OPTS*sizeof(char*));
	int nOpt = 0;
	size_t size = 0;

	// iterate on the file until fgets returns NULL (EOF)
	while(getline(&lineBuf, &size , configFile) >= 0){

		// strip comments from the line	
		strip((char*)lineBuf, COMMENT_CHAR);

		// this check ignores lines which are fully commented
		// (they do not contain an option)
		/*if(lineBuf[0] != '\0' && lineBuf[0] != '\n'){*/
		size_t sl = strlen(lineBuf);
		if(sl <= 2*SIZEBUF-2 && sl > 1){
			// scan the stripped string for options
			// copy them into placeholder arrays
			sscanf(lineBuf, "%s %s", optBuf, valBuf);	
			// remove quotes
			if(!valid_opt(optBuf)){
				/*fprintf(stderr,"Invalid option in config file: %s\nValid format is:\noption: value\n", optBuf);*/
				destroy_mat(opt, nOpt);
				destroy_mat(val, nOpt);
				fclose(configFile);
				errno = EINVAL;
				return -1;
			}
			strip((char*)optBuf, SEPARATOR);
			// add format to option field (--option)
			format_option((char*)optBuf);
			opt[nOpt] = STRDUP(optBuf);
			val[nOpt] = STRDUP(valBuf);
			// to avoid character mess
			memset(optBuf, 0, SIZEBUF);
			nOpt++;
			FREE(lineBuf);
			size = 0;
		}
	}
	FREE(lineBuf);
	size = 2*nOpt + 1;
	retOpts = create_opts_vector(size);
	fill_opts_vector(retOpts, size, opt, val, 0);
	int i;
	for (i=0; i<size-1; i=i+2){
		printf("%s %s\n", retOpts[i], retOpts[i+1]);
	}
	destroy_mat(opt, N_OPTS);
	destroy_mat(val, N_OPTS);
	fclose(configFile);

	assert (nOpt == N_OPTS); // did we parse all the options?

	*returnOpts = retOpts; // now the callee has the options
	return nOpt; // number of options read // should be always 11
}

