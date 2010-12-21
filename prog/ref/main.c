/*
 * main.c - Extract vectors from official reference and check implementation
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "chips.h"

#include "../vectors.h"


#define END_VECTOR ((uint32_t) ~0)


#include DATA_FILE


static struct map {
    const char *vector_name;	/* NULL if last */
    const char *chip_name;	/* NULL if any */
    const uint32_t *vectors;
    uint16_t data;
} map[] = {
#include MAP_FILE
    { NULL, NULL, NULL, 0 }
};


static struct vector {
    const char *name;		/* NULL if last */
    const char *chips;		/* NULL if any */
    const char *vector;
} vectors[] = {
#include VECTOR_FILE
    { NULL, NULL, NULL }
};


static const char *lookup(const char *vector,const char *chip)
{
    const struct vector *v;

    for (v = vectors; v->name; v++)
	if (!strcmp(v->name,vector)) {
	    if (!v->chips || !chip)
		return v->vector;
	    if (strstr(v->chips,chip))
		return v->vector;
	}
    fprintf(stderr,"vector \"%s\" (%s) not found\n",vector,chip ? chip : "*");
    exit(1);
}


static void check_map(void)
{
    const struct map *m;

    for (m = map; m->vector_name; m++) {
	const char *v,*p;
	const uint32_t *n;
	int bits;

	v = lookup(m->vector_name,m->chip_name);
	bits = 0;
	for (p = v; *p; p++)
	    if (*p != '0' && *p != '1' && *p != 'Z')
		bits++;
	assert(!(strlen(v) % 22));
	p = v;
	for (n = m->vectors; *n != END_VECTOR; n++) {
	    uint32_t a,b = 0;
	    int i;

	    if (!*n)
		a = 0;
	    else if (IS_WRITE(*n)) {
		/* ...111 */
		a = (*n & 0x7ffff) << 3;
		if (m != map || n != m->vectors)
		    a |= 7;
	    }
	    else {
		if (bits < 8) {
		    fprintf(stderr,"\"%s\" (%s) [%d]: out of bits\n",
		      m->vector_name,m->chip_name ? m->chip_name : "*",
		      (int) (n-m->vectors));
		    exit(1);
		}
		bits -= 8;
		/* ...Z<d*8>Z1 */
		a = (*n & 0x7ff00) << 3;
		a |= ((m->data >> bits) & 0xff) << 2;
		a |= 1;
	    }
	    for (i = 0; i != 22; i++) {
		if (!*p) {
		    fprintf(stderr,"\"%s\" (%s) [%d]: string too short\n",
		      m->vector_name,m->chip_name ? m->chip_name : "*",
		      (int) (n-m->vectors));
		    exit(1);
	        }
		if (*p == '0' || *p == 'Z' || *p == 'L' || *p == 'D')
		    b <<= 1;
		else if (*p == '1' || *p == 'H')
		    b = (b << 1) | 1;
		else {
		    if (!bits) {
			fprintf(stderr,"\"%s\" (%s) [%d]: out of bits\n",
			  m->vector_name,m->chip_name ? m->chip_name : "*",
			  (int) (n-m->vectors));
			exit(1);
		    }
		    bits--;
		    b = (b << 1) | ((m->data >> bits) & 1);
		}
		p++;
	    }
	    if (a != b) {
		fprintf(stderr,"\"%s\" (%s) [%d]: ref 0x%x != 0x%0x\n",
		  m->vector_name,m->chip_name ? m->chip_name : "*",
		  (int) (n-m->vectors),a,b);
		exit(1);
	    }
	}
	if (bits) {
	    fprintf(stderr,"\"%s\" (%s) [%d]: %d bits left\n",
	      m->vector_name,m->chip_name ? m->chip_name : "*",
	      (int) (n-m->vectors),bits);
	    exit(1);
	}
	if (*p) {
	    fprintf(stderr,"\"%s\" (%s) [%d]: tail \"%s\"\n",
	      m->vector_name,m->chip_name ? m->chip_name : "*",
	      (int) (n-m->vectors),p);
	    exit(1);
	}
    }
}


static void dump_vectors(void)
{
    const struct vector *v;

    for (v = vectors; v->name; v++) {
	char buf[2000];

	printf("# %s (%s)\n",v->name,v->chips ? v->chips : "*");
	fflush(stdout);
	sprintf(buf,"echo %s | perl ./vdecode.pl",v->vector);
	system(buf);
    }
}


int main(int argc,char **argv)
{
    const char *v;

    switch (argc-1) {
	case 0:
	    check_map();
	    break;
	case 1:
	    if (!strcmp(argv[1],"-d")) {
		dump_vectors();
		break;
	    }
	    /* fall through */
	case 2:
	    v = lookup(argv[1],argv[2]);
	    printf("%s\n",v);
	    break;
	default:
	    fprintf(stderr,"usage: %s [vector_name [chip_name]]\n",*argv);
	    fprintf(stderr,"       %s -d\n",*argv);
	    fprintf(stderr,"       %s\n",*argv);
	    return 1;
    }
    return 0;
}
