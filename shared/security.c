/*
 * security.c - Check protection bits
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "file.h"
#include "chips.h"
#include "security.h"


void set_security(int block,int mode)
{
    security[block/4] |= mode << 2*(block & 3);
}


void read_security(const char *name)
{
    static const char modes[] = "UuFfRrWw";
    static const char *stdin_name = "stdin";
    FILE *file;
    const char *s;
    int block = 0,comment = 0;
    int strong = 0;
    int c,n;

    if (!strcmp(name,"-")) {
	file = stdin;
	name = stdin_name;
    }
    else {
	file = fopen(name,"r");
	if (!file) {
	    perror(name);
	    exit(1);
	}
    }
    memset(security,0,SECURITY_SIZE);
    while ((c = fgetc(file)) != EOF) {
	if (comment) {
	    if (c == '\n')
		comment = 0;
	    continue;
	}
	if (c == ';') {
	    comment = 1;
	    continue;
	}
	if (isspace(c))
	    continue;
	s = strchr(modes,c);
	if (!s || !c) {
	    fprintf(stderr,"%s: unrecognized protection mode\n",name);
	    exit(1);
	}
	if (block == SECURITY_SIZE*4) {
	    fprintf(stderr,"too many protection bits\n");
	    exit(1);
	}
	n = (s-modes) >> 1;
	strong |= n > 1;
	set_security(block,n);
	block++;
    }
    if (name != stdin_name)
	if (fclose(file) == EOF) {
	    perror(name);
	    exit(1);
	}
    if (block & 3) {
	fprintf(stderr,"warning: padding incomplete protection byte with %c\n",
	    strong ? 'R' : 'U');
	while (block & 3) {
	    set_security(block,strong ? 2 : 0);
	    block++;
	}
    }
    security_size = block/4;
}


/*
 * We check the permissions for the following potential problems:
 *
 * - any block with mode F can be read through the checksum function
 * - any block with mode U or F can be used for code injection attacks
 * - blocks with unspecified protection must be write-protected if
 *   read-protection is desired through R or W
 * - even if the Flash size is not known, we know that it must be be a power of
 *   two, so we enforce at least this (note: this currently does nothing,
 *   because we pad to a multiple of 256 blocks anyway, but if there will ever
 *   be a 64 kB part, one could trigger this condition)
 *
 * The following only applies to some chips (CY8C24223 and CY8C24423A tested
 * positive, CY8C21323 and CY8C29466 tested negative). To err on the safe side,
 * we warn for all chips with one Flash bank.
 *
 * - all protection bits in a bank must be set, or writes may wrap, while the
 *   protection check doesn't
 * - likewise, if such wrapping occurs, the protection of mirrored blocks must
 *   not be weaker than the original
 */

void check_security(const struct chip *chip)
{
    int i,j;
    int warn_f = 1;
    int weak = 0,strong = 0,all_zero = 1;

    for (i = 0; i != security_size; i++)
	for (j = 0; j != 8; j += 2)
	    switch ((security[i] >> j) & 3) {
		case 1:
		    if (warn_f)
			fprintf(stderr,
			  "warning: protection mode F does not protect\n");
		    warn_f = 0;
		    all_zero = 0;
		    /* fall through */
		case 0:
		    weak = 1;
		    break;
		case 2:
		case 3:
		    strong = 1;
		    all_zero = 0;
		    break;
		default:
		    abort();
	    }
    if (all_zero)
	return;
    if (weak && strong)
	fprintf(stderr,
	  "warning: modes U and F compromise read protection of R and W\n");
    if (chip) {
	if (chip->banks*chip->blocks > security_size*4) {
	    fprintf(stderr,
	      "warning: padding protection bits to full Flash size with %c\n",
	      strong ? 'R' : 'U');
	    while (chip->banks*chip->blocks != security_size*4)
		security[security_size++] = strong ? 0xaa : 0;
	}
	if (chip->banks == 1) {
	    while (security_size != BLOCK_SIZE) {
		security[security_size] =
		  security[security_size & (chip->blocks/4-1)];
		security_size++;
	    }
	    for (i = 0; i != BLOCK_SIZE; i++)
		for (j = 0; j != 4; j++)
		    if (((security[i] >> 2*j) & 3) <
			((security[i & (chip->blocks/4-1)] >> 2*j) & 3)) {
			fprintf(stderr,
			  "warning: protection of mirrored Flash is weaker "
			  "than original\n");
			goto out;
		    }
	    out:
		;
	}
    }
    if (security_size & (BLOCK_SIZE-1)) {
	fprintf(stderr,"warning: padding protection bits with %c\n",
	  strong ? 'R' : 'U');
	while (security_size & (BLOCK_SIZE-1))
	    security[security_size++] = strong ? 0xaa : 0;
    }
    for (i = security_size; !(i & 1); i >>= 1);
    if (i != 1)
	fprintf(stderr,
	  "warning: number of protection bits is not a power of two\n");
}
