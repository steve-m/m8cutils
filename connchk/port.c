/*
 * port.c - Port setup and access
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "prog.h"
#include "interact.h"
#include "vectors.h"

#include "value.h"
#include "out.h"
#include "test.h"
#include "port.h"


#define MAGIC		0x5c


uint64_t defined;

static uint64_t dm0,dm1,dm2,dr;
static uint64_t cached_dm0,cached_dm1,cached_dm2,cached_dr;


/* ----- Register access --------------------------------------------------- */


static void set_xio(int addr)
{
    static int xio = 0;

    if (addr >> 8 != xio) {
	xio = !xio;
	prog_vectors(WRITE_REG(CPU_F,xio ? CPU_F_XIO : 0));
    }
}


static void write_reg(int addr,uint8_t data)
{
    set_xio(addr);
    prog_vectors(WRITE_REG(addr & 0xff,data));
}


uint8_t read_reg(int addr)
{
    set_xio(addr);
    return prog_vectors(READ_REG(addr & 0xff));
}


/* ----- Port configuration bits ------------------------------------------- */


int decode_value(int bit)
{
    static const uint8_t value[] = {
	VALUE_0R, VALUE_1,
	VALUE_0,  VALUE_1,
	VALUE_Z,  VALUE_Z,
	VALUE_0,  VALUE_1R,
	VALUE_Z,  VALUE_1,
	VALUE_0,  VALUE_1,
	VALUE_Z,  VALUE_Z,
	VALUE_0,  VALUE_Z,
    };

    return value[(((dm2 >> bit) & 1) << 3) |
      (((dm1 >> bit) & 1) << 2) |
      (((dm0 >> bit) & 1) << 1) |
      ((dr >> bit) & 1)];
}


static void set_bit(uint64_t *set,int bit,int value)
{
    *set = ((*set) & ~((uint64_t) 1 << bit)) | ((uint64_t) value << bit);
}


static void set_dm_bits(int bit,int value)
{
    set_bit(&dm2,bit,value >> 2);
    set_bit(&dm1,bit,(value >> 1) & 1);
    set_bit(&dm0,bit,value & 1);
}


static void set_dr_bit(int bit,int value)
{
    set_bit(&dr,bit,value);
}


void set_value(int bit,int value)
{
    switch (value) {
	case VALUE_Z:
	    set_dm_bits(bit,2);
	    break;
	case VALUE_0R:
	    set_dm_bits(bit,0);
	    set_dr_bit(bit,0);
	    break;
	case VALUE_1R:
	    set_dm_bits(bit,3);
	    set_dr_bit(bit,1);
	    break;
	case VALUE_0:
	    set_dm_bits(bit,1);
	    set_dr_bit(bit,0);
	    break;
	case VALUE_1:
	    set_dm_bits(bit,1);
	    set_dr_bit(bit,1);
	    break;
	default:
	    abort();
    }
}


/* ----- ISSP communication check ------------------------------------------ */


void check_target(int current_bit,int current_value)
{
    if (prog_vectors(READ_MEM(0)) != MAGIC)
	lost_comm(current_bit,current_value);
}


/* ----- Propagate port settings to the target ----------------------------- */


static void change(int port,int addr,uint64_t old,uint64_t new)
{
    uint8_t old_byte = old >> port*8;
    uint8_t new_byte = new >> port*8;

    if (old_byte != new_byte)
	write_reg(addr,new_byte);
}


/*
 * First move towards DM210 = 110, then to the final value. This way, we won't
 * do any illegal transitions, e.g., 0R -> 1 with a transient 0.
 */

void commit(int current_bit,int current_value)
{
    int i;

    for (i = 0; i != 8; i++)
	if ((defined >> i*8) & 0xff) {
	    change(i,PRTxDMy(i,2),cached_dm2,cached_dm2 | dm2);
	    change(i,PRTxDMy(i,0),cached_dm0,cached_dm0 & dm0);
	    change(i,PRTxDMy(i,1),cached_dm1,cached_dm1 | dm1);
	    change(i,PRTxDR(i),cached_dr,dr);
	}
    cached_dm0 &= dm0;
    cached_dm1 |= dm1;
    cached_dm2 |= dm2;
    cached_dr = dr;

    for (i = 0; i != 8; i++)
	if ((defined >> i*8) & 0xff) {
	    change(i,PRTxDMy(i,2),cached_dm2,dm2);
	    change(i,PRTxDMy(i,0),cached_dm0,dm0);
	    change(i,PRTxDMy(i,1),cached_dm1,dm1);
	}
    cached_dm0 = dm0;
    cached_dm1 = dm1;
    cached_dm2 = dm2;

    check_target(current_bit,current_value);
}


void init_ports(void)
{
    int i;

    /* set up reset conditions */
    for (i = 0; i != 64; i++)
	dm1 |= (uint64_t) 1 << i;
    dm2 = cached_dm1 = cached_dm2 = dm1;

    /* write magic number for checking whether we've lost the target */
    prog_vectors(WRITE_MEM(0,MAGIC));
}
