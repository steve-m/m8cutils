/*
 * chips.h - CY8C2xxxx chip database
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef CHIPS_H
#define CHIPS_H


#include <stdint.h>
#include <stdio.h>


#define ID_CY8C27143	0x0009
#define	ID_CY8C27243	0x000a
#define	ID_CY8C27443	0x000b
#define ID_CY8C27543	0x000c
#define ID_CY8C27643	0x000d
#define ID_CY8C24123	0x0012
#define	ID_CY8C24223	0x0013
#define ID_CY8C24423	0x0014
#define ID_CY8C24423A	0x0034
#define ID_CY8C22113	0x000f
#define ID_CY8C22213	0x0010
#define ID_CY8C21123	0x0017
#define ID_CY8C21223	0x0018
#define	ID_CY8C21323	0x0019
#define ID_CY8C21234	0x0036
#define	ID_CY8C21334	0x0037
#define	ID_CY8C21434	0x0038
#define	ID_CY8C21534	0x0040
#define ID_CY8C21634	0x0049
#define ID_CY8C24794	0x001d
#define ID_CY8C29466	0x002a
#define ID_CY8C29566	0x002b
#define ID_CY8C29666	0x002c
#define ID_CY8C29866	0x002d


struct chip {
    const char *name;
    uint16_t id;
    int banks;		/* ROM banks, 64*block bytes each */
    int blocks;		/* ROM blocks per bank, 64 bytes each */
    int pages;		/* RAM pages, 256 bytes each */
    int cy8c27xxx;
};


/*
 * chip_by_name and chip_by_id return NULL if no corresponding entry is found.
 */

void chip_list(FILE *file,int width);
const struct chip *chip_by_name(const char *name);
const struct chip *chip_by_id(const uint16_t id);

#endif /* !CHIPS_H */
