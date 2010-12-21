/*
 * ice.c - Use of programming mode as an ICE
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "m8c.h"

#include "vectors.h"
#include "chips.h"
#include "prog_vector.h"
#include "ice.h"


#undef OUTPUT_VECTOR
#define OUTPUT_VECTOR(v) (v)	/* we use "raw" vectors */


static int xio = 0;
static uint8_t f,pcl,pch,code0,code1,code2;


/*
 * The clocks (probably just the IMO) are stopped after each ISSP access.
 * Executing some code revives them. Note: this seems very similar to sleep,
 * but unfortunately, it can't be cleared by just playing with the Sleep bit in
 * CPU_SCR0.
 */

static void prepare_start(void)
{
    if (pch != 0)
	prog_vector(WRITE_REG(CPU_PCH,0));
    if (pcl != 3)
    if (code0 != OP_HALT || code1 != OP_NOP || code2 != OP_NOP) {
	if (f & CPU_F_XIO)
	    prog_vector(WRITE_REG(CPU_F,f & ~CPU_F_XIO));
	if (code0 != OP_HALT)
	    prog_vector(WRITE_REG(CPU_CODE0,OP_HALT));
	if (code1 != OP_NOP)
	    prog_vector(WRITE_REG(CPU_CODE1,OP_NOP));
	if (code2 != OP_NOP)
	    prog_vector(WRITE_REG(CPU_CODE2,OP_NOP));
	if (f & CPU_F_XIO)
	    prog_vector(WRITE_REG(CPU_F,f));
    }
}


static void start_clock(void)
{
    prog_vector(WRITE_REG(CPU_PCL,3));
    prog_vector(EXEC_VECTOR_VALUE);
    prog_vector(ZERO_VECTOR);
}


static void set_shadows(void)
{
    if (pch != 0)
	prog_vector(WRITE_REG(CPU_PCH,pch));
    /* it's safer not to try to predict the value of CPU_PCL */
    prog_vector(WRITE_REG(CPU_PCL,pcl));
    if (code0 != OP_HALT || code1 != OP_NOP || code2 != OP_NOP) {
	if (f & CPU_F_XIO)
	    prog_vector(WRITE_REG(CPU_F,f & ~CPU_F_XIO));
	if (code0 != OP_HALT)
	    prog_vector(WRITE_REG(CPU_CODE0,code0));
	if (code1 != OP_NOP)
	    prog_vector(WRITE_REG(CPU_CODE1,code1));
	if (code2 != OP_NOP)
	    prog_vector(WRITE_REG(CPU_CODE2,code2));
	if (f & CPU_F_XIO)
	    prog_vector(WRITE_REG(CPU_F,f));
    }
}


static void fetch_shadows(void)
{
    f = prog_vector(READ_REG(CPU_F));
    if (f & CPU_F_XIO)
	prog_vector(WRITE_REG(CPU_F,f & ~CPU_F_XIO));
    pch = prog_vector(READ_REG(CPU_PCH));
    pcl = prog_vector(READ_REG(CPU_PCL));
    code0 = prog_vector(READ_REG(CPU_CODE0));
    code1 = prog_vector(READ_REG(CPU_CODE1));
    code2 = prog_vector(READ_REG(CPU_CODE2));
    if (f & CPU_F_XIO)
	prog_vector(WRITE_REG(CPU_F,f));
}


static void set_xio(int reg)
{
    if (reg >> 8 != xio) {
	xio = !xio;
	prog_vector(WRITE_REG(CPU_F,xio ? CPU_F_XIO : 0));
    }
}


void ice_init(void)
{
    f = prog_vector(READ_REG(CPU_F));
    fetch_shadows();
    prepare_start();
    start_clock();
}


uint8_t ice_read(int reg)
{
    uint8_t value;

    switch (reg) {
	case CPU_PCH:
	case CPU_PCH | 0x100:
	    return pch;
	case CPU_PCL:
	case CPU_PCL | 0x100:
	    return pcl;
	case CPU_CODE0:
	    return code0;
	case CPU_CODE1:
	    return code1;
	case CPU_CODE2:
	    return code2;
	default:
	    set_xio(reg);
	    value = prog_vector(READ_REG(reg & 0xff));
	    start_clock();
	    return value;
    }
}


void ice_write(int reg,uint8_t value)
{
    switch (reg) {
	case CPU_PCH:
	case CPU_PCH | 0x100:
	    pch = value;
	    break;
	case CPU_PCL:
	case CPU_PCL | 0x100:
	    pcl = value;
	    break;
	case CPU_CODE0:
	    code0 = value;
	    break;
	case CPU_CODE1:
	    code1 = value;
	    break;
	case CPU_CODE2:
	    code2 = value;
	    break;
	case CPU_SCR0:
	    if (value == MAGIC_EXEC) {
		set_shadows();
		if (code0 == OP_SSC)
		    prog_vector(EXEC_VECTOR_SSC_VALUE);
		else {
		    prog_vector(EXEC_VECTOR_VALUE);
		    prog_vector(ZERO_VECTOR);
		}
		fetch_shadows();
		prepare_start();
		break;
	    }
	    /* fall through */
	default:
	    set_xio(reg);
	    prog_vector(WRITE_REG(reg & 0xff,value));
    }
    start_clock();
}
