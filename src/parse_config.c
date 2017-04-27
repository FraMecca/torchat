/* A tokenizer that can parse the config file 
 * at the moment it is very simple, reads the file and formats the options
 * it supports comments (default char is #)
 * To be included in torchat, by changing main(void) declaration
 * standalone compile with:
 * gcc parse_config.c ../include/mem.c ../include/except.c ../include/ut_assert.c  -o pc -Wall -g
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // exit
#include "../include/mem.h"

// the character that will identify comments
// and the number of options in the config file
#define COMMENT_CHAR '#'
#define N_OPTS 11

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
	char tmp[100] = {0};
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

int
main(void)
{
	FILE *configFile = fopen("../torchat.conf", "r");
	if (!configFile){
		fprintf(stderr, "Unable to open config file.\n");
		exit(1);
	}

	char lineBuf[512], optBuf[100] = {0}, valBuf[100] = {0};
	char **retOpts;
	// opt is used to store the formatted values of option
	// val is for the values
	char **opt = MALLOC(N_OPTS*sizeof(char*));
	char **val = MALLOC(N_OPTS*sizeof(char*));
	int nOpt = 0;
	int size;

	// iterate on the file until fgets returns NULL (EOF)
	while(fgets(lineBuf, 512, configFile) != NULL){

		// strip comments from the line	
		strip((char*)lineBuf, COMMENT_CHAR);

		// this check ignores lines which are fully commented
		// (they do not contain an option)
		if (lineBuf[0] != '\0' && lineBuf[0] != '\n'){
			// scan the stripped string for options
			// copy them into placeholder arrays
			sscanf(lineBuf, "%s %s", optBuf, valBuf);	
			// remove quotes
			strip((char*)optBuf, ':');
			// add format to option field (--option)
			format_option((char*)optBuf);
			opt[nOpt] = STRDUP(optBuf);
			val[nOpt] = STRDUP(valBuf);
			// to avoid character mess
			memset(optBuf, 0, strlen(optBuf));
			nOpt++;
		}
	}

	size = 2*nOpt + 1;
	retOpts = create_opts_vector(size);
	fill_opts_vector(retOpts, size, opt, val, 0);
	int i;
	for (i=0; i<size-1; i=i+2){
		printf("%s = %s\n", retOpts[i], retOpts[i+1]);
	}
	destroy_mat(opt, N_OPTS);
	destroy_mat(val, N_OPTS);
	fclose(configFile);
	destroy_mat(retOpts, size);
	return 0;
}

