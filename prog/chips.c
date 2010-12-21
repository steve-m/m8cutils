/*
 * chips.c - CY8C2xxxx chip database
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m8cprog.h"
#include "vectors.h"
#include "chips.h"


#define ID(id) #id, ID_##id


const struct chip chips[] = {
    { ID(CY8C21123), 1,  64, 0, },
    { ID(CY8C21223), 1,  64, 0, },
    { ID(CY8C21234), 1, 128, 0, },
    { ID(CY8C21323), 1,  64, 0, },
    { ID(CY8C21334), 1, 128, 0, },
    { ID(CY8C21434), 1, 128, 0, },
    { ID(CY8C21534), 1, 128, 0, },
    { ID(CY8C21634), 1, 128, 0, },
    { ID(CY8C22113), 1,  32, 0, },
    { ID(CY8C22213), 1,  32, 0, },
    { ID(CY8C24123), 1,  64, 0, },
    { ID(CY8C24223), 1,  64, 0, },
    { ID(CY8C24423), 1,  64, 0, },
    { ID(CY8C27143), 1, 256, 1, },
    { ID(CY8C27243), 1, 256, 1, },
    { ID(CY8C27443), 1, 256, 1, },
    { ID(CY8C27543), 1, 256, 1, },
    { ID(CY8C27643), 1, 256, 1, },
    { ID(CY8C24794), 2, 128, 0, },
    { ID(CY8C29466), 4, 128, 0, },
    { ID(CY8C29566), 4, 128, 0, },
    { ID(CY8C29666), 4, 128, 0, },
    { ID(CY8C29866), 4, 128, 0, },
    { NULL, }
};


void chip_list(FILE *file)
{
    const struct chip *c;
    int col = 0;

    for (c = chips; c->name; c++) {
	if (strlen(c->name)+col+10 > OUTPUT_WIDTH) {
	    putc('\n',file);
	    col = 0;
	}
	fprintf(file,"%s (0x%04x)%s",c->name,c->id,c[1].name ? ", " : "");
	col += strlen(c->name)+10;
    }
    putc('\n',file);
}


const struct chip *chip_by_name(const char *name)
{
    const struct chip *c;

    for (c = chips; c->name; c++)
	if (!strcasecmp(c->name,name))
	    break;
    return c->name ? c : NULL;
}


const struct chip *chip_by_id(const uint16_t id)
{
    const struct chip *c;

    for (c = chips; c->name; c++)
	if (c->id == id)
	    break;
    return c->name ? c : NULL;
}
