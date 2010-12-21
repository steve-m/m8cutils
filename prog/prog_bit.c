/*
 * prog_bit.c - Bit-banging operations
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include "prog_bit.h"


struct prog_bit prog_bit;


void prog_bit_init(const struct prog_bit *bit)
{
    if (bit)
	prog_bit = *bit;
}
