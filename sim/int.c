/*
 * int.c - Interrupt handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>

#include "m8c.h"
#include "ice.h"

#include "util.h"
#include "reg.h"
#include "core.h"
#include "sim.h"
#include "int.h"


uint8_t int_vc = 0;

static const int int2prio[] = {
     1,  2,  3,  4,  5,  7, 25,  6,
     8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24
};

static const int prio2int[] = {
    -1,  0,  1,  2,  3,  4,  7,  5,
     8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 6
};


static uint32_t int_pending;	/* pending interrupts */
static uint32_t int_msk;
static uint32_t int_line;	/* line status, for edge detection */
static uint32_t int_ice;	/* interrupts handled by the ICE */


/* ----- Handle the current interrupt -------------------------------------- */


static void int_select(void)
{
    if (int_pending & int_msk)
	int_vc = int2prio[ctz(int_pending & int_msk)]*4;
    else
	int_vc = 0;
    if (int_ice) {
	uint8_t tmp;

	tmp = ice_read(INT_VC);
	if ((tmp & 3) || tmp > 0x64) {
	    fprintf(stderr,
	      "fatal communication error: ICE sent INT_VC = 0x%x\n",tmp);
	    exit(1);
	}
	if (tmp && (!int_vc || (tmp < int_vc)))
	    int_vc = tmp;
    }
}


void int_handle(void)
{
    int int_num;

    stack[sp++] = pc >> 8;
    stack[sp++] = pc;
    stack[sp++] = f;
    regs[CPU_F].ops->sim_write(regs+CPU_F,0);
    pc = int_vc;
    int_num = prio2int[int_vc/4];
    int_pending &= ~(1 << int_num);
    if (int_ice) {
	uint8_t int_msk3 = int_msk >> 24;

	if (int_msk3 & INT_MSK3_ENSWINT)
	    ice_write(INT_MSK3,int_msk3 & ~INT_MSK3_ENSWINT);
	ice_write(INT_CLR0+int_num/8,~(1 << (int_num & 3)));
	if (int_msk3 & INT_MSK3_ENSWINT)
	    ice_write(INT_MSK3,int_msk3);
    }
    int_select();
}


/*
 * @@@ To do: build an "intelligent" programmer that polls locally and sends an
 * interrupt to the host if it detects something.
 */

void int_poll(void)
{
    if (int_ice)
	int_select();
}


/* ----- INT_CLRx ---------------------------------------------------------- */


static uint8_t int_read_vc(struct reg *reg)
{
    return int_vc;
}


static void int_write_vc(struct reg *reg,uint8_t value)
{
    int_vc = 0;
    int_pending = 0;
    if (int_ice)
	ice_write(INT_VC,0);
}


static const struct reg_ops int_vc_ops = {
    .cpu_read = int_read_vc,
    .cpu_write = int_write_vc,
};


/* ----- INT_CLRx ---------------------------------------------------------- */


static uint8_t int_read_clr(struct reg *reg)
{
    unsigned long n = 8*(unsigned long) reg->user;
    uint8_t pending,ice_mask;

    pending = int_pending >> n;
    ice_mask = int_ice >> n;
    if (ice_mask)
	pending |= ice_read(reg-regs) & ice_mask;
    return pending;
}


static void int_write_clr(struct reg *reg,uint8_t value)
{
    unsigned long n = 8*(unsigned long) reg->user;

    /*
     * FIXME: if the interrupt source still sends an H, software-generated
     * interrupts aren't supposed to work, says the TRM.
     */
    if (int_msk & (INT_MSK3_ENSWINT << 24))
	int_pending |= (value << n) & ~int_line;
    else
	int_pending &= ~(value << n);
    if (int_ice)
	ice_write(reg-regs,value & (int_ice >> n));
    int_select();
}


static const struct reg_ops int_clr_ops = {
    .cpu_read = int_read_clr,
    .cpu_write = int_write_clr,
};


/* ----- INT_MSKx ---------------------------------------------------------- */


static int msk_n2reg(int n)
{
    switch (n) {
	case 0:
	    return INT_MSK0;
	case 1:
	    return INT_MSK1;
	case 2:
	    return INT_MSK2;
	case 3:
	    return INT_MSK3;
	default:
	    abort();
    }
}


static uint8_t int_read_msk(struct reg *reg)
{
    return int_msk >> (8*(unsigned long) reg->user);
}


static void int_write_msk(struct reg *reg,uint8_t value)
{
    unsigned long n = 8*(unsigned long) reg->user;

    int_msk = (~(0xff << n) & int_msk) | (value << n);
    if (int_ice)
	ice_write(reg-regs,value & (int_ice >> n));
    int_select();
}


static const struct reg_ops int_msk_ops = {
    .cpu_read = int_read_msk,
    .cpu_write = int_write_msk,
};


/* ----- Simulation (script or internal) raises interrupt ------------------ */


void int_set(int number)
{
    if (!((int_line >> number) & 1)) {
	int_pending |= 1 << number;
	int_select();
    }
    int_line |= 1 << number;
}


void int_clear(int number)
{
    int_line &= ~(1 << number);
}


void int_set_ice(int number,int connected)
{
    if (connected) {
	if (int_ice & (1 << number))
	    return;
	int_ice |= 1 << number;
    }
    else {
	if (~int_ice & (1 << number))
	    return;
	int_ice &= ~(1 << number);
    }
    ice_write(msk_n2reg(number/8),int_ice >> (number & ~7));
}


/* ----- Initialization ---------------------------------------------------- */


static void setup_int(int reg)
{
    regs[INT_CLR0+reg].ops = &int_clr_ops;
    regs[msk_n2reg(reg)].ops = &int_msk_ops;
    regs[INT_CLR0+reg].user = (void *) (unsigned long) reg;
    regs[msk_n2reg(reg)].user = (void *) (unsigned long) reg;
}


void int_init(void)
{
    int i;

    regs[INT_VC].ops = &int_vc_ops;
    for (i = 0; i != 4; i++)
	setup_int(i);
    int_pending = int_line = int_msk = int_ice = 0;
}
