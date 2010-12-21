/*
 * interact.h - Definitions and functions for controling user interaction
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef INTERACT_H
#define INTERACT_H

#include <stdio.h>


#define DEFAULT_OUTPUT_WIDTH	80


extern int verbose;	/* verbosity level (default 0) */
extern int quiet;	/* no positive feedback or progress (default 0) */


/*
 * get_output_width returns the terminal width (in characters), or 0 if there
 * is no terminal associated with the specified file.
 */

int get_output_width(FILE *file);

void progress(FILE *file,const char *label,int n,int end);
void progress_clear(FILE *file);

#endif /* !INTERACT_H */
