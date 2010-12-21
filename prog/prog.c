/*
 * prog.c - Interface to the programmer hardware
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cy8c2prog.h"
#include "vectors.h"
#include "prog.h"


PROGRAMMERS_DECL

struct prog_ops *programmers[] = {
    PROGRAMMERS_OPS
    NULL
};


static struct prog_ops *prog = NULL;
static int initial = 0;


void prog_list(FILE *file)
{
    struct prog_ops **walk;
    int col = 0;

    for (walk = programmers; *walk ; walk++) {
	if (strlen((*walk)->name)+col+2 > OUTPUT_WIDTH) {
	    putc('\n',file);
	    col = 0;
	}
	fprintf(file,"%s%s",(*walk)->name,walk[1] ? ", " : "");
	col += strlen((*walk)->name)+2;
    }
    putc('\n',file);
}


int prog_open(const char *dev,const char *programmer,int voltage)
{
    struct prog_ops **walk;

    for (walk = programmers; *walk ; walk++) {
	prog = *walk;
	if (!strcasecmp(prog->name,programmer))
	    break;
    }
    if (!*walk) {
	fprintf(stderr,"programmer \"%s\" is not known\n",programmer);
	exit(1);
    }
    voltage = prog->open(dev,voltage);
    if (!voltage) {
	fprintf(stderr,
	  "must specify voltage (-3 or -5) with this programmer\n");
	exit(1);
    }
    initial = 1;
    return voltage;
}


uint8_t prog_vector(uint32_t v)
{
    int value = 0;
    int i;

    if (verbose > 1) {
	fprintf(stderr,"VECTOR 0x%08x: ",v);
	if (!v)
	    fprintf(stderr,"ZEROES\n");
	else {
	    if (IS_WRITE(v))
		fprintf(stderr,"WRITE %s 0x%02x,0x%02x%s\n",
		  IS_REG(v) ? "REG" : "MEM",(v >> 8) & 0xff,v & 0xff,
		  IS_SSC(v) ? " (SSC)" : "");
	    else
		fprintf(stderr,"READ %s 0x%02x\n",
		  IS_REG(v) ? "REG" : "MEM",(v >> 8) & 0xff);
	}
    }
    if (prog->vector) {
	value = prog->vector(v);
	if (!IS_WRITE(v) && verbose > 1)
	    fprintf(stderr,"-> 0x%02x\n",value);
	return value;
    }
    if (!v) {
	for (i = 0; i != 22; i++)
	    prog->send_bit(0);
    }
    else if (IS_WRITE(v)) {
	for (i = 18; i >= 0; i--)
	    prog->send_bit((v >> i) & 1);
	for (i = 0; i != 3; i++)
	    prog->send_bit(1);
    }
    else {
	for (i = 18; i >= 8; i--)
	    prog->send_bit((v >> i) & 1);
	prog->send_z();
	for (i = 0; i != 8; i++) {
	    prog->send_z();
	    if (prog->read_bit())
		value |= 1 << (7-i);
	}
	prog->send_z();
	prog->send_bit(1);
	if (verbose > 1)
	    fprintf(stderr,"-> 0x%02x\n",value);
    }
    if (IS_SSC(v)) {
	prog->send_z();
	usleep(100);
	while (prog->read_bit());
	for (i = 0; i != 40; i++)
	    prog->send_bit(0);
    }
    return value;
}


uint32_t do_prog_vectors(uint32_t v,...)
{
    uint32_t result = 0;
    va_list ap;

    va_start(ap,v);
    while (v != END_OF_VECTORS) {
	if (IS_WRITE(v))
	    prog_vector(v);
	else
	    result = (result << 8) | prog_vector(v);
	v = va_arg(ap,uint32_t);
    }
    va_end(ap);
    return result;
}


void prog_close(void)
{
    if (prog->close)
	prog->close();
}
