/*
 * ice.c - Use of programming mode as an ICE
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "vectors.h"
#include "chips.h"
#include "prog.h"
#include "ops.h"
#include "ice.h"


#undef OUTPUT_VECTOR
#define OUTPUT_VECTOR(v) (v)	/* we use "raw" vectors */


static int xio = 0;


const struct chip *ice_open(const char *dev,const char *programmer,
  const char *chip_name,int voltage)
{
    const struct chip *chip;

    if (chip_name) {
	chip = chip_by_name(chip_name);
        if (!chip) {
	    fprintf(stderr,"chip \"%s\" is not known\n",chip_name);
	    exit(1);
	}
    }
    voltage = prog_open(dev,programmer,voltage);
    prog_initialize(0,voltage);
    return prog_identify(chip);
    
}


static void set_xio(int reg)
{
    if (reg >> 8 != xio) {
	xio = !xio;
	prog_vector(WRITE_REG(REG_CPU_F,xio ? CPU_F_XIO : 0));
    }
}


uint8_t ice_read(int reg)
{
    set_xio(reg);
    return prog_vector(READ_REG(reg & 0xff));
}


void ice_write(int reg,uint8_t value)
{
    set_xio(reg);
    prog_vector(WRITE_REG(reg & 0xff,value));
}


void ice_close(void)
{
    prog_close();
}
