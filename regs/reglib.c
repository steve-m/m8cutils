/*
 * reglib.h - Helper functions for register definitions
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <string.h>

#include "regdef.h"


int psoc_regdef_find_chip(const char *name)
{
    const struct psoc_regdef_chip *chip;

    for (chip = proc_regdef_chips; chip->name; chip++)
	if (!strcasecmp(chip->name,name))
	    return chip->ind;
    return -1;
}


int psoc_regdef_applicable(const struct psoc_regdef_register *reg,int chip)
{
    if (chip == -1)
	return 1;
    return reg->chips[chip] == '1';
}
