/*
 * reginfo.c - Print register structure and decode values
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "regdef.h"


#define BIT_WIDTH		9	/* characters per bit */
#define NO_VALUE		~0UL
#define FIELD_ALWAYS_NUMERIC	3	/* always print value for big fields */


static void digit(int pos,int val,char sep)
{
    printf("%*s%d%*s%c",(BIT_WIDTH-1)/2,"",val,BIT_WIDTH/2,"",
      pos ? sep : '\n');
}


static void field(int pos,const char *buf,int bits)
{
    int w = bits*BIT_WIDTH+bits-1;

    w -= strlen(buf);
    printf("%*s%s%*s%c",w/2,"",buf,(w+1)/2,"",pos-bits ? '|' : '\n');
}


static void line(void)
{
    int i,j;

    for (i = 0; i != 8; i++) {
	for (j = 0; j != BIT_WIDTH; j++)
	    putchar('-');
	putchar(i == 7 ? '\n' : '+');
    }
}


static void usage(const char *name)
{
    fprintf(stderr,"usage: %s register [value]\n",name);
    exit(1);
}


int main(int argc,char *argv[])
{
    const struct psoc_regdef_register *reg;
    unsigned long value = NO_VALUE;
    char buf[20];
    int i;

    switch (argc) {
	char *end;
	unsigned long addr;

	case 3:
	    value = strtoul(argv[2],&end,0);
	    if (*end || value & ~0xff)
		usage(*argv);
	    /* fall through */
	case 2:
	    addr = strtoul(argv[1],&end,0);
	    if (*end) {
		for (reg = psoc_regdef; reg->name; reg++)
		    if (!strcasecmp(reg->name,argv[1]))
			goto found;
		fprintf(stderr,"no register called \"%s\"\n",argv[1]);
		exit(1);
	    }
	    else {
		if (addr & ~0x1ff)
		    usage(*argv);
		for (reg = psoc_regdef; reg->name; reg++)
		    if (reg->addr == addr)
			goto found;
		fprintf(stderr,"no register at 0x%lx\n",addr);
		exit(1);
	    }
	    break;
	default:
	    usage(*argv);
    }

found:
    printf("%s: %c,%02Xh\n\n",reg->name,
      reg->addr >> 8 ? '1' : '0',reg->addr & 0xff);
    for (i = 7; i >= 0; i--)
	digit(i,i,' ');
    line();
    if (reg->fields) {
	const struct psoc_regdef_field *f = reg->fields;

	for (i = 8; i; i -= (f++)->bits)
	    field(i,f->name ? f->name : "",f->bits);
	line();
    }
    if (value != NO_VALUE) {
	for (i = 7; i >= 0; i--)
	    digit(i,(value >> i) & 1,'|');
	line();
	if (reg->fields) {
	    const struct psoc_regdef_field *f = reg->fields;

	    for (i = 8; i; i -= (f++)->bits) {
		uint8_t n;

		n = (value & ((1 << i)-1)) >> (i-f->bits);
		if (!f->values)
		    sprintf(buf,"0x%X",n);
		else {
		    const struct psoc_regdef_value *v = f->values;

		    while (v->name && v->value != n)
			v++;
		    if (!v->name)
			sprintf(buf,"(0x%X)",n);
		    else {
			if (f->bits < FIELD_ALWAYS_NUMERIC)
			    strcpy(buf,v->name);
			else
			    sprintf(buf,"%s (%d)",v->name,n);
		    }
		}
		field(i,f->name ? buf : "-",f->bits);
	    }
	}
	else {
	    sprintf(buf,"0x%02lX",value);
	    field(8,buf,8);
	}
	line();
    }
    return 0;
}
