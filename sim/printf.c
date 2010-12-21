/*
 * printf.c - Simple printf function
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "sim.h"
#include "printf.h"


void do_printf(FILE *file,const char *fmt,const struct nlist *args)
{
    static char f[] = "%?*?";
    const char *p;
    int zero = 0;
    int width = 0;

    for (p = fmt; *p; p++) {
	if (*p != '%') {
	    fputc(*p,file);
	    continue;
	}
	if (*++p == '%') {
	    fputc('%',file);
	    continue;
	}
	if (!args)
	    yyerror("not enough printf arguments");
	zero = *p == '0';
	if (zero)
	    p++;
	while (isdigit(*p))
	    width = width*10+*p++-'0';
	if (!strchr("cduoxX",*p))
	    yyerrorf("unknown format character \"%c\"",*p);
	f[1] = zero ? '0' : '%';
	f[3] = *p;
	fprintf(file,zero ? f : f+1,width,args->n);
	args = args->next;
    }
    if (args)
	yyerror("extra printf arguments");
    fflush(file);
}
