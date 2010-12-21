/*
 * gpio.c - General-purpose I/O
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "ice.h"

#include "registers.h"
#include "reg.h"
#include "gpio.h"


static struct gpio {
    uint8_t dr;
    uint8_t dm0;
    uint8_t dm1;
    uint8_t dm2;
    uint8_t ice;
    uint8_t drive;
} gpios[8];


/* ----- PRTxDR ------------------------------------------------------------ */


static inline uint8_t is_input(const struct gpio *gpio)
{
    return (gpio->dm1 & ~gpio->dm0) |
      (~(gpio->dm0 ^ gpio->dm1) & ~(gpio->dm1 & gpio->dr));
}


static uint8_t gpio_read_dr(struct reg *reg)
{
    struct gpio *gpio = reg->user;
    uint8_t input,dr,ice;

    input = is_input(gpio);
    /* @@@ not entirely correct: if port is forced, input may be != output */
    if (!input)
	return gpio->dr;
    dr = (gpio->dr & ~input) | (gpio->drive & input);
    if (!gpio->ice)
	return dr;
    ice = ice_read(reg-regs);
    return (dr & ~gpio->ice) | (ice & gpio->ice);
}


static void gpio_write_dr(struct reg *reg,uint8_t value)
{
    struct gpio *gpio = reg->user;

    if (gpio->ice)
	ice_write(reg-regs,value);
    gpio->dr = value;
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
}


static const struct reg_ops gpio_dm2_ops = {
    .cpu_read = gpio_read_dm2,
    .cpu_write = gpio_write_dm2,
};


/* ----- Target connection ------------------------------------------------- */


void gpio_ice_connect(int port,uint8_t set)
{
    struct gpio *gpio = gpios+port;

    if ((gpio->ice & set) == set)
	return;
    /* @@@ ugly hard-coded setting */
    if (port == 1 && (set & 3)) {
	fprintf(stderr,"P1[0] and P1[1] are reserved for the ICE\n");
	exit(1);
    }
    gpio->ice |= set;
    /* @@@ figure out a good sequence */
    ice_write(PRT0DR+4*port,gpio->dr);
    ice_write(PRT0DM0+4*port,gpio->dm0 & gpio->ice);
    ice_write(PRT0DM1+4*port,gpio->dm1 | ~gpio->ice);
    ice_write(PRT0DM2+4*port,gpio->dm2 | ~gpio->ice);
}


void gpio_ice_disconnect(int port,uint8_t set)
{
    struct gpio *gpio = gpios+port;

    gpio->ice &= ~set;
    if (gpio->dm0 & ~gpio->ice)
	ice_write(PRT0DM0+4*port,gpio->dm0 & gpio->ice);
    if ((gpio->dm1 | gpio->ice) != 0xff)
	ice_write(PRT0DM1+4*port,gpio->dm1 | ~gpio->ice);
    if ((gpio->dm2 | gpio->ice) != 0xff)
	ice_write(PRT0DM1+4*port,gpio->dm2 | ~gpio->ice);
}


/* ----- Simulation-provided drive ----------------------------------------- */


void gpio_drive(int port,int bit,uint8_t mask,uint8_t value)
{
    struct gpio *gpio = gpios+port;

    /* @@@ interrupt */
    gpio->drive = (gpio->drive & ~mask) | (value & mask);
}


/* ----- Initialization --------------------------------------------------0- */


static void setup_gpio(int port)
{
    int i;

    regs[PRT0DR+4*port].ops = &gpio_dr_ops;
    regs[PRT0DM0+4*port].ops = &gpio_dm0_ops;
    regs[PRT0DM1+4*port].ops = &gpio_dm1_ops;
    regs[PRT0DM2+4*port].ops = &gpio_dm2_ops;
    for (i = 0; i != 4; i++)
	regs[PRT0DR+4*port+i].user = regs[PRT0DM0+4*port+i].user = gpios+port;
    gpios[port].dm0 = 0;
    gpios[port].dm1 = gpios[port].dm2 = 0xff;
    gpios[port].ice = 0;
    gpios[port].drive = 0;
}


void gpio_init(void)
{
    setup_gpio(0);
    setup_gpio(1);
    setup_gpio(2);
    setup_gpio(3);
    setup_gpio(4);
    setup_gpio(5);
    setup_gpio(6);
    setup_gpio(7);
}
