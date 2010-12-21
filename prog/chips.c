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

#include "cy8c2prog.h"
#include "vectors.h"
#include "chips.h"


#define ID(id) #id, ID_##id


const struct chip chips[] = {
    { ID(CY8C21123),  64, 0, },
    { ID(CY8C21223),  64, 0, },
    { ID(CY8C21234), 128, 0, },
    { ID(CY8C21323),  64, 0, },
    { ID(CY8C21334), 128, 0, },
    { ID(CY8C21434), 128, 0, },
    { ID(CY8C21534), 128, 0, },
    { ID(CY8C21634), 128, 0, },
    { ID(CY8C22113),  32, 0, },
    { ID(CY8C22213),  32, 0, },
    { ID(CY8C24123),  64, 0, },
    { ID(CY8C24223),  64, 0, },
    { ID(CY8C24423),  64, 0, },
    { ID(CY8C27143), 256, 1, },
    { ID(CY8C27243), 256, 1, },
    { ID(CY8C27443), 256, 1, },
    { ID(CY8C27543), 256, 1, },
    { ID(CY8C27643), 256, 1, },
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
