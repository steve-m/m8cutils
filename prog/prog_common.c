/*
 * prog_common.c - Common programmer operations and driver selection
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "interact.h"

#include "prog_common.h"


PROGRAMMERS_DECL

struct prog_common *programmers[] = {
    PROGRAMMERS_OPS
    NULL
};


struct prog_common prog_common;


void prog_list(FILE *file)
{
    struct prog_common **walk;
    int width,col = 0;

    width = get_output_width(file);
    if (!width)
	width = DEFAULT_OUTPUT_WIDTH;
    for (walk = programmers; *walk ; walk++) {
	if (strlen((*walk)->name)+col+2 >= width) {
	    putc('\n',file);
	    col = 0;
	}
	fprintf(file,"%s%s",(*walk)->name,walk[1] ? ", " : "");
	col += strlen((*walk)->name)+2;
    }
    putc('\n',file);
}


/* ----- Opening ----------------------------------------------------------- */


int prog_open(const char *dev,const char *programmer,int voltage,int power_on,
   const char *args[])
{
    const struct prog_common *prog;

    if (!dev)
	dev = getenv("M8CPROG_PORT");
    if (!programmer)
	programmer = getenv("M8CPROG_DRIVER");
    if (!programmer)
	prog = *programmers;
    else {
	struct prog_common **walk;

	for (walk = programmers; *walk ; walk++) {
	    prog = *walk;
	    if (!strcasecmp(prog->name,programmer))
		break;
	}
	if (!*walk) {
	    fprintf(stderr,"programmer \"%s\" is not known\n",programmer);
	    exit(1);
	}
    }
    prog_common_init(prog);
    voltage = prog->open(dev,voltage,power_on,args);
    if (voltage < 0) {
	fprintf(stderr,"programming mode not supported\n");
	exit(1);
    }
    return voltage;
}


static void common_initialize(int power_on)
{
}


/* ----- Closing ----------------------------------------------------------- */



static void common_detach(void)
{
    fprintf(stderr,"warning: \"detach\" operation is not supported\n");
    prog_common.close();
}


static void common_close(void)
{
    /* nothing to do */
}


void prog_close(int detach)
{
    if (detach)
	prog_common.detach();
    else
	prog_common.close();
}


/* ----- Populate prog_common ---------------------------------------------- */


#define COMPLETE(name) \
    if (!prog_common.name) \
	prog_common.name = common_##name


void prog_common_init(const struct prog_common *common)
{
    prog_common = *common;
    if (!prog_common.open) {
	fprintf(stderr,"prog_common_init: mandatory functions are missing\n");
	exit(1);
    }
    COMPLETE(initialize);
    COMPLETE(close);
    COMPLETE(detach);
}
