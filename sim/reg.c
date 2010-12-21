/*
 * reg.c - Register handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>

#include "util.h"
#include "sim.h"
#include "reg.h"


struct reg regs[NUM_REGS];


/* ----- Simple read access ------------------------------------------------ */


uint8_t read_user(struct reg *reg)
{                         
    return *(uint8_t *) reg->user;
}


/* ----- Generic register (without side-effects) --------------------------- */


struct generic_data {
    uint8_t value;
    uint8_t mask;
};


static uint8_t generic_cpu_read(struct reg *reg)
{
    struct generic_data *data = (struct generic_data *) reg->user;

    return data->value;
}


static void generic_cpu_write(struct reg *reg,uint8_t value)
{
    struct generic_data *data = (struct generic_data *) reg->user;

    data->value = (data->value & ~data->mask) | (value & data->mask);
}


static void generic_sim_write(struct reg *reg,uint8_t value)
{
    struct generic_data *data = (struct generic_data *) reg->user;

    data->value = value;
}


static const struct reg_ops generic_ops = {
    .cpu_read = generic_cpu_read,
    .cpu_write = generic_cpu_write,
    .sim_write = generic_sim_write,
};


void generic_reg(int n,uint8_t value,uint8_t mask)
{
    struct generic_data *data = alloc_type(struct generic_data);

    regs[n].ops = &generic_ops;
    regs[n].user = data;
    data->value = value;
    data->mask = mask;
}


/* ----- Forbidden register ------------------------------------------------ */


uint8_t forbidden_read(struct reg *reg)
{
    int n = reg-regs;

    exception("Read from forbidden register %d,%02xh",n >> 8,n & 0xff);
    return 0;
}


void forbidden_write(struct reg *reg,uint8_t value)
{
    int n = reg-regs;

    exception("Write 0x%02x to forbidden register %d,%02xh (%s)",
      value,n >> 8,n & 0xff,reg->name ? reg->name : "unknown");
}


static const struct reg_ops forbidden_ops = {
    .cpu_read = forbidden_read,
    .cpu_write = forbidden_write,
};


static void forbidden_reg(int n)
{
    regs[n].ops = &forbidden_ops;
}


/* ----- Register setup ---------------------------------------------------- */


void init_regs(void)
{
    int i;

    for (i = 0; i != NUM_REGS; i++)
	if (!regs[i].ops)
	    forbidden_reg(i);
}
