/*
 * gpio.c - General-purpose I/O
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define CY8C21323	/* arbitrary non-USB chip */
#include "m8c.h"
#include "ice.h"
#include "interact.h"

#include "util.h"
#include "reg.h"
#include "int.h"
#include "sim.h"
#include "gpio.h"


static struct gpio {
    uint8_t dr;		/* data register */
    uint8_t dm0;	/* drive mode */
    uint8_t dm1;
    uint8_t dm2;
    uint8_t ie;		/* interrupt enable */
    uint8_t ic0;	/* interrupt control */
    uint8_t ic1;
    uint8_t ice;	/* pin is connected to ICE */
    uint8_t latch;	/* last value read */
    uint8_t drive;	/* input value in simulator */
    uint8_t drive_z;	/* Z, overrides "drive" and "drive_r" */
    uint8_t drive_r;	/* resistive, modifies "drive" */
    int int_set;	/* GPIO interrupt is asserted because of this port */
} gpios[8];


/* ----- Input helper ------------------------------------------------------ */


/*
 * We don't do anything special for cases where external drive and the pin's
 * drive mode end up in a tie, such as strong H against strong L, resistive H
 * against resistive L, or Hi-Z against Hi-Z.
 */


static inline uint8_t is_input(const struct gpio *gpio)
{
    return (gpio->dm1 & ~gpio->dm0) |
      (((~gpio->drive_r & ~gpio->drive_z) | gpio->dm2 | gpio->ice) &
      ~(gpio->dm0 ^ gpio->dr) & ~(gpio->dm1 ^ gpio->dr));

}


/* ----- Interrupts -------------------------------------------------------- */


static void gpio_maybe_interrupt(struct gpio *gpio)
{
    uint8_t input,dr;

    input = is_input(gpio);
    dr = (gpio->dr & ~input) | (gpio->drive & input);
    if (gpio->ie & ~gpio->ice &
      ((~gpio->ic1 & gpio->ic0 & ~dr) |
      (gpio->ic1 & ~gpio->ic0 & dr) |
      (gpio->ic1 & gpio->ic0 & (dr ^ gpio->latch)))) {
	gpio->int_set = 1;
	int_set(ctz(INT_MSK0_GPIO));
    }
    else {
	int i;

	gpio->int_set = 0;
	for (i = 0; i != 8; i++)
	    if (gpio->int_set)
		return;
	int_clear(ctz(INT_MSK0_GPIO));
    }
}


/* ----- PRTxDR ------------------------------------------------------------ */


static uint8_t gpio_read_dr(struct reg *reg)
{
    struct gpio *gpio = reg->user;
    uint8_t input,dr;

    input = is_input(gpio);
//fprintf(stderr,"input 0x%x R0x%x Z0x%x\n",input,gpio->drive_r,gpio->drive_z);
    /*
     * Not talking to the ICE if the port is an input isn't entirely correct:
     * if the port is forced high or low, this may even override a "strong"
     * output.
     */
    dr = (gpio->dr & ~input) | (gpio->drive & input);
    if (!(input & gpio->ice)) {
	gpio->latch = dr;
	return dr;
    }
    dr = (dr & ~gpio->ice) | (ice_read(reg-regs) & gpio->ice);
    gpio->latch = dr;
    return dr;
}


static void gpio_write_dr(struct reg *reg,uint8_t value)
{
    struct gpio *gpio = reg->user;

    if (gpio->ice)
	ice_write(reg-regs,value & gpio->ice);
    gpio->dr = value;
    gpio_maybe_interrupt(gpio);
}


static const struct reg_ops gpio_dr_ops = {
    .cpu_read = gpio_read_dr,
    .cpu_write = gpio_write_dr,
};


/* ----- PRTxDM0 ----------------------------------------------------------- */


static uint8_t gpio_read_dm0(struct reg *reg)
{
    struct gpio *gpio = reg->user;

    return gpio->dm0;
}


static void gpio_write_dm0(struct reg *reg,uint8_t value)
{
    struct gpio *gpio = reg->user;

    if (gpio->ice)
	ice_write(reg-regs,value & gpio->ice);
    gpio->dm0 = value;
    gpio_maybe_interrupt(gpio);
}


static const struct reg_ops gpio_dm0_ops = {
    .cpu_read = gpio_read_dm0,
    .cpu_write = gpio_write_dm0,
};


/* ----- PRTxDM1 ----------------------------------------------------------- */


static uint8_t gpio_read_dm1(struct reg *reg)
{
    struct gpio *gpio = reg->user;

    return gpio->dm1;
}


static void gpio_write_dm1(struct reg *reg,uint8_t value)
{
    struct gpio *gpio = reg->user;

    if (gpio->ice)
	ice_write(reg-regs,value | ~gpio->ice);
    gpio->dm1 = value;
    gpio_maybe_interrupt(gpio);
}


static const struct reg_ops gpio_dm1_ops = {
    .cpu_read = gpio_read_dm1,
    .cpu_write = gpio_write_dm1,
};


/* ----- PRTxDM2 ----------------------------------------------------------- */


static uint8_t gpio_read_dm2(struct reg *reg)
{
    struct gpio *gpio = reg->user;

    return gpio->dm2;
}


static void gpio_write_dm2(struct reg *reg,uint8_t value)
{
    struct gpio *gpio = reg->user;

    if (gpio->ice)
	ice_write(reg-regs,value | ~gpio->ice);
    gpio->dm2 = value;
    /*
     * We don't call gpio_maybe_interrupt here, because none of the PRTxDM2
     * changes could cause a GPIO interrupt in the simulator. (They may on
     * real hardware, e.g., if using an external pull-up or -down which is
     * weaker than the internal one, so it can affect the pin while in Hi-Z,
     * but not when resistive.)
     */
}


static const struct reg_ops gpio_dm2_ops = {
    .cpu_read = gpio_read_dm2,
    .cpu_write = gpio_write_dm2,
};


/* ----- PRTxIE ------------------------------------------------------------ */


static uint8_t gpio_read_ie(struct reg *reg)
{
    struct gpio *gpio = reg->user;

    return gpio->ie;
}


static void gpio_write_ie(struct reg *reg,uint8_t value)
{
    struct gpio *gpio = reg->user;

    if (gpio->ice)
	ice_write(reg-regs,value & gpio->ice);
    gpio->ie = value;
    gpio_maybe_interrupt(gpio);
}


static const struct reg_ops gpio_ie_ops = {
    .cpu_read = gpio_read_ie,
    .cpu_write = gpio_write_ie,
};


/* ----- PRTxIC0 ----------------------------------------------------------- */


static uint8_t gpio_read_ic0(struct reg *reg)
{
    struct gpio *gpio = reg->user;

    return gpio->ic0;
}


static void gpio_write_ic0(struct reg *reg,uint8_t value)
{
    struct gpio *gpio = reg->user;

    if (gpio->ice)
	ice_write(reg-regs,value & gpio->ice);
    gpio->ic0 = value;
    gpio_maybe_interrupt(gpio);
}


static const struct reg_ops gpio_ic0_ops = {
    .cpu_read = gpio_read_ic0,
    .cpu_write = gpio_write_ic0,
};


/* ----- PRTxIC1 ----------------------------------------------------------- */


static uint8_t gpio_read_ic1(struct reg *reg)
{
    struct gpio *gpio = reg->user;

    return gpio->ic0;
}


static void gpio_write_ic1(struct reg *reg,uint8_t value)
{
    struct gpio *gpio = reg->user;

    if (gpio->ice)
	ice_write(reg-regs,value & gpio->ice);
    gpio->ic1 = value;
    gpio_maybe_interrupt(gpio);
}


static const struct reg_ops gpio_ic1_ops = {
    .cpu_read = gpio_read_ic1,
    .cpu_write = gpio_write_ic1,
};


/* ----- Target connection ------------------------------------------------- */


/*
 * The PTRxDMx bits are arranged such that a transition out of analog Hi-Z/Hi-Z
 * (110) can be done by changing the bits in any order, without creating
 * undesirable conditions (e.g., Hi-Z to Resisitve to Strong, or, worse, Hi-Z
 * to Strong to Hi-Z).
 *
 * Furthermore, analog Hi-Z and regular Hi-Z are not distinguished. I.e., data
 * can be read in either mode.
 */

void gpio_ice_connect(int port,uint8_t set)
{
    struct gpio *gpio = gpios+port;

    if ((gpio->ice & set) == set)
	return;
    /* @@@ ugly hard-coded setting */
    if (port == 1 && (set & 3)) {
	fprintf(stderr,"P1[0] and P1[1] are reserved for the ICE\n");
	exit_if_script(1);
	return;
    }
    gpio->ice |= set;
    ice_write(PRT0DR+4*port,gpio->dr);
    ice_write(PRT0DM0+4*port,gpio->dm0 & gpio->ice);
    ice_write(PRT0DM1+4*port,gpio->dm1 | ~gpio->ice);
    ice_write(PRT0DM2+4*port,gpio->dm2 | ~gpio->ice);
    ice_write(PRT0IC0+4*port,gpio->ic0 & gpio->ice);
    ice_write(PRT0IC1+4*port,gpio->ic1 & gpio->ice);
    /*
     * Do PRTxIE last, to avoid interrupts from transient PRTxICx state.
     */
    ice_write(PRT0IE+4*port,gpio->ie & gpio->ice);
    int_set_ice(ctz(INT_CLR0_GPIO),1);
}


/*
 * We drive disconnected pins back to analog Hi-Z.
 */

void gpio_ice_disconnect(int port,uint8_t set)
{
    struct gpio *gpio = gpios+port;
    int i;

    gpio->ice &= ~set;
    for (i = 0; i != 8; i++)
	if (gpio[i].ice)
	    goto have_ice;
    int_set_ice(ctz(INT_CLR0_GPIO),0);
have_ice:
    if (gpio->dm0 & ~gpio->ice)
	ice_write(PRT0DM0+4*port,gpio->dm0 & gpio->ice);
    if ((gpio->dm1 | gpio->ice) != 0xff)
	ice_write(PRT0DM1+4*port,gpio->dm1 | ~gpio->ice);
    if ((gpio->dm2 | gpio->ice) != 0xff)
	ice_write(PRT0DM2+4*port,gpio->dm2 | ~gpio->ice);
    /*
     * Do PRTxIE first, to avoid interrupts from transient PRTxICx state.
     */
    if (gpio->ie & ~gpio->ice)
	ice_write(PRT0IE+4*port,gpio->ie & gpio->ice);
    if (gpio->ic0 & ~gpio->ice)
	ice_write(PRT0IC0+4*port,gpio->ic0 & gpio->ice);
    if (gpio->ic1 & ~gpio->ice)
	ice_write(PRT0IC1+4*port,gpio->ic1 & gpio->ice);
}


/* ----- Simulation-provided drive ----------------------------------------- */


void gpio_drive(int port,uint8_t mask,uint8_t value)
{
    struct gpio *gpio = gpios+port;

    gpio->drive = (gpio->drive & ~mask) | (value & mask);
    gpio->drive_r &= ~mask;
    gpio->drive_z &= ~mask;
    gpio_maybe_interrupt(gpio);
}


void gpio_drive_r(int port,uint8_t mask,uint8_t value)
{
    struct gpio *gpio = gpios+port;

    gpio->drive = (gpio->drive & ~mask) | (value & mask);
    gpio->drive_r |= mask;
    gpio->drive_z &= ~mask;
    gpio_maybe_interrupt(gpio);
}


void gpio_drive_z(int port,uint8_t mask)
{
    struct gpio *gpio = gpios+port;

    gpio->drive_z |= mask;
    gpio_maybe_interrupt(gpio);
}


/* ----- Set drive mode and data ------------------------------------------- */


static void gpio_may_change(int port,int reg,const uint8_t *curr,uint8_t mask,
  uint8_t value)
{
    uint8_t new = (*curr & ~mask) | value;

    assert(!(value & ~mask));
    if (*curr == new)
	return;
    reg_write(regs+reg+4*port,new);
    if (verbose > 1)
	gpio_show(stderr,port,mask);
}


void gpio_set(int port,uint8_t mask,uint8_t value)
{
    struct gpio *gpio = gpios+port;

    gpio_may_change(port,PRT0DR,&gpio->dr,mask,value);
    gpio_may_change(port,PRT0DM0,&gpio->dm0,mask,mask);
    gpio_may_change(port,PRT0DM1,&gpio->dm1,mask,0);
}


/*
 * Setting resistive modes is tricky, because we have more forbidden states
 * than in all the other cases. The algorithm below yields correct results,
 * but it may be possible to optimize it by reducing the number of bits
 * changed in the first two conditional transitions.
 */

void gpio_set_r(int port,uint8_t mask,uint8_t value)
{
    struct gpio *gpio = gpios+port;
    uint8_t dm0,dm1,dm2,dr;

    dm0 = (gpio->dm0 & ~mask) | value;
    dm1 = (gpio->dm1 & ~mask) | value;
    dm2 = gpio->dm2 & ~mask;
    dr = (gpio->dr & ~mask) | value;

    /* only change transitions out of DR = 1 */
    gpio_may_change(port,PRT0DM0,&gpio->dm0,mask,
      (gpio->dm0 & ~gpio->dr) | (dm0 & gpio->dr));

    /* only change transitions out of DR = 0 */
    gpio_may_change(port,PRT0DM1,&gpio->dm1,mask,
      (gpio->dm1 & gpio->dr) | (dm1 & ~gpio->dr));

    gpio_may_change(port,PRT0DM2,&gpio->dm2,mask,dm2);
    gpio_may_change(port,PRT0DR,&gpio->dr,mask,dr);
    gpio_may_change(port,PRT0DM0,&gpio->dm0,mask,dm0);
    gpio_may_change(port,PRT0DM1,&gpio->dm1,mask,dm1);
}


void gpio_set_z(int port,uint8_t mask)
{
    struct gpio *gpio = gpios+port;

    gpio_may_change(port,PRT0DM0,&gpio->dm0,mask,0);
    gpio_may_change(port,PRT0DM1,&gpio->dm1,mask,mask);
    gpio_may_change(port,PRT0DM2,&gpio->dm2,mask,0);
}


void gpio_set_analog(int port,uint8_t mask)
{
    struct gpio *gpio = gpios+port;

    gpio_may_change(port,PRT0DM0,&gpio->dm0,mask,0);
    gpio_may_change(port,PRT0DM1,&gpio->dm1,mask,mask);
    gpio_may_change(port,PRT0DM2,&gpio->dm2,mask,mask);
}


/* ----- Port status ------------------------------------------------------- */


static const char *gpio_state[] = {
    "0R",	"0",		"Z",		"0",
    "Z",	"0(slow)",	"analog",	"0(slow)",
    "1",	"1",		"Z",		"1R",
    "1(slow)",	"1(slow)",	"analog",	"Z"
};


static void gpio_show_pin(FILE *file,const struct gpio *gpio,int n)
{
    int dm,dr;

    dm = ((gpio->dm2 >> n) & 1) << 2 |
      ((gpio->dm1 >> n) & 1) << 1 |
      ((gpio->dm0 >> n) & 1);
    dr = (gpio->dr >> n) & 1;
    fprintf(file,"P%d[%d] DM=%d%d%d DR=%d  %s",(int) (gpio-gpios),n,
      dm >> 2,(dm >> 1) & 1,dm & 1,dr,
      gpio_state[dm | dr << 3]);
    if ((gpio->ice >> n) & 1)
	fprintf(file,"  <-> ICE\n");
    else if (gpio->drive_z & (1 << n))
	fputc('\n',file);
    else {
	fprintf(file,"  <-- drive %d%s\n",
	  (gpio->drive >> n) & 1,
	  gpio->drive_r & (1 << n) ? " R" : "");
    }
}


void gpio_show(FILE *file,int port,uint8_t mask)
{
    const struct gpio *gpio = gpios+port;
    int i;

    for (i = 0; i != 8; i++) {
	if (!((mask >> i) & 1))
	    continue;
	gpio_show_pin(file,gpio,i);
    }
}


/* ----- Initialization --------------------------------------------------0- */


static void setup_gpio(int port)
{
    int i;

    regs[PRT0DR+4*port].ops = &gpio_dr_ops;
    regs[PRT0DM0+4*port].ops = &gpio_dm0_ops;
    regs[PRT0DM1+4*port].ops = &gpio_dm1_ops;
    regs[PRT0DM2+4*port].ops = &gpio_dm2_ops;
    regs[PRT0IE+4*port].ops = &gpio_ie_ops;
    regs[PRT0IC0+4*port].ops = &gpio_ic0_ops;
    regs[PRT0IC1+4*port].ops = &gpio_ic1_ops;
    for (i = 0; i != 4; i++)
	regs[PRT0DR+4*port+i].user = regs[PRT0DM0+4*port+i].user = gpios+port;
    gpios[port].dm0 = 0;
    gpios[port].dm1 = gpios[port].dm2 = 0xff;
    gpios[port].ie = gpios[port].ic0 = gpios[port].ic1 = 0;
    gpios[port].latch = 0;
    gpios[port].ice = 0;
    gpios[port].drive = gpios[port].drive_r = 0;
    gpios[port].drive_z = 0xff;
}


void gpio_init(void)
{
    int i;

    for (i = 0; i != 8; i++)
	setup_gpio(i);
}
