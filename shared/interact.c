/*
 * interact.c - Definitions and functions for controling user interaction
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

#include "interact.h"


int verbose = 0;
int quiet = 0;


static int output_fd = -1;
static int output_width;  /* 0 if output disabled */
static int last_hash = -1;  /* number of hash signs, -1 for none */


int get_output_width(FILE *file)
{
    struct winsize winsz;

    if (output_fd == fileno(file))
	return output_width;
    output_fd = fileno(file);
    if (ioctl(output_fd,TIOCGWINSZ,&winsz) < 0) {
	if (errno != ENOTTY) {
	    perror("ioctl(TIOCGWINSZ)");
	    exit(1);
	}
        output_width = 0;
    }
    else {
	output_width = winsz.ws_col ? winsz.ws_col : DEFAULT_OUTPUT_WIDTH;
    }
    return output_width;
}


/*
 * verbose > 1 is supposed to provide more details than a progress bar, so we
 * disable the latter, for it would only get in the way.
 */

void progress(FILE *file,const char *label,int n,int end)
{
    static int width,left;
    int hash;

    if (quiet || verbose > 1)
        return;
    if (last_hash == -1) {
	width = get_output_width(file);
	if (!width)
	    return;
	left = width-strlen(label)-2;
	fprintf(file,"\r%s ",label);
	last_hash = 0;
    }
    hash = left*((n+0.0)/end);
    while (last_hash != hash) {
	fputc('#',file);
	last_hash++;
    }
}


void progress_clear(FILE *file)
{
    int width;

    if (quiet || verbose > 1)
        return;
    width = get_output_width(file);
    if (width)
	fprintf(file,"\r%*s\r",width-1,"");
    last_hash = -1;
}

