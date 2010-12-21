/*
 * symmap.c - Symbol map reader
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "symmap.h"


#define MAX_LINE	16384


struct symbol {
    const char *name;
    uint32_t value;
    int attr;
} *map = NULL;


static const struct symbol **by_name,**by_value;
static int entries = 0;


static int comp_value(const void *a,const void *b)
{
    const struct symbol *sym_a = *(const struct symbol **) a;
    const struct symbol *sym_b = *(const struct symbol **) b;
    uint32_t va = sym_a->value;
    uint32_t vb = sym_b->value;

    return va < vb ? -1 : va > vb ? 1 : 0;
}


static int comp_name(const void *a,const void *b)
{
    const struct symbol *sym_a = *(const struct symbol **) a;
    const struct symbol *sym_b = *(const struct symbol **) b;

    return strcmp(sym_a->name,sym_b->name);
}


void sym_read_file(FILE *file)
{
    char buf[MAX_LINE];
    int line = 0;
    int i;

    while (fgets(buf,MAX_LINE,file)) {
	char *here,*name;
	char space[4],scope;
	int attr;

	line++;
	here = strchr(buf,'#');
	if (here)
	    *here = 0;
	if (strspn(buf," \t\r\n") == strlen(buf))
	    continue;
	map = realloc(map,sizeof(struct symbol)*(entries+1));
	if (!map) {
	    perror("realloc");
	    exit(1);
	}
	if (sscanf(buf,"%3s %x %c %as",
	  space,&map[entries].value,&scope,&name) != 4) {
	    fprintf(stderr,"error in symbol map, line %d\n",line);
	    exit(1);
	}
	if (!strcmp(space,"ROM"))
	    attr = SYM_ATTR_ROM;
	else if (!strcmp(space,"RAM"))
	    attr = SYM_ATTR_RAM;
	else if (!strcmp(space,"REG"))
	    attr = SYM_ATTR_REG;
	else if (!strcmp(space,"EQU"))
	    attr = SYM_ATTR_EQU;
	else {
	    fprintf(stderr,"unrecognized address space \"%s\" at line %d\n",
	      space,line);
	    exit(1);
	}
	if (scope == 'G')
	    attr |= SYM_ATTR_GLOBAL;
	else if (scope == 'L')
	    attr |= SYM_ATTR_LOCAL;
	else {
	    fprintf(stderr,"unrecognized symbol scope \"%c\" at line %d\n",
	      scope,line);
	    exit(1);
	}
	map[entries].name = name;
	  /* doing this directly in fscanf confuses older versions of gcc */
	map[entries].attr = attr;
	entries++;
    }
    by_name = malloc(entries*sizeof(struct symbol *));
    by_value = malloc(entries*sizeof(struct symbol *));
    if (!by_name || !by_value) {
	perror("malloc");
	exit(1);
    }
    for (i = 0; i != entries; i++)
	by_name[i] = by_value[i] = map+i;
    qsort(by_name,entries,sizeof(struct symbol *),comp_name);
    qsort(by_value,entries,sizeof(struct symbol *),comp_value);
}


void sym_read_file_by_name(const char *name)
{
    FILE *file;

    if (!strcmp(name,"-"))
	file = stdin;
    else {
	file = fopen(name,"r");
	if (!file) {
	    perror(name);
	    exit(1);
	}
    }
    sym_read_file(file);
    if (file != stdin)
	(void) fclose(file);
}


static const struct symbol *find_symbol(const struct symbol *key,
  const struct symbol **base,int (*comp)(const void *a,const void *b),
  int attr_mask,int *attr)
{
    const struct symbol **hit;
    const struct symbol *found = NULL;

    hit = bsearch(&key,base,entries,sizeof(struct symbol *),comp);
    if (!hit)
	return NULL;
    while (hit != base && !(*comp)(&key,hit-1))
	hit--;
    while (hit != base+entries && !(*comp)(&key,hit)) {
	if ((*hit)->attr & attr_mask) {
	    if (found) {
		if (attr)
		    *attr |= SYM_ATTR_MORE;
		return found;
	    }
	    found = *hit;
	    if (attr)
		*attr = found->attr;
	}
	hit++;
    }
    return found;
}


const uint32_t *sym_by_name(const char *name,int attr_mask,int *attr)
{
    struct symbol key;
    const struct symbol *symbol;

    key.name = name;
    symbol = find_symbol(&key,by_name,comp_name,attr_mask,attr);
    return symbol ? &symbol->value : NULL;
}


const char *sym_by_value(uint32_t value,int attr_mask,int *attr)
{
    struct symbol key;
    const struct symbol *symbol;

    key.value = value;
    symbol = find_symbol(&key,by_value,comp_value,attr_mask,attr);
    return symbol ? symbol->name: NULL;
}
