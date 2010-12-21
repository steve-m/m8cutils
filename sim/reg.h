/*
 * reg.h - Register handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

#ifndef REG_H
#define REG_H

#include <stdint.h>

#include "chips.h"


#define NUM_REGS 0x200

/*
 * cpu_* are accesses from the CPU running in the simulator. sim_* are accesses
 * from the simulation control script. If sim_read and sim_write are NULL, the
 * corresponding cpu_* function is used. If it is also NULL, the simulator is
 * not supposed to access this register (e.g., because it doesn't exist).
 */

struct reg;

struct reg_ops {
    uint8_t (*cpu_read)(struct reg *reg);
    void (*cpu_write)(struct reg *reg,uint8_t value);
    uint8_t (*sim_read)(struct reg *reg);		/* may be NULL */
    void (*sim_write)(struct reg *reg,uint8_t value);	/* may be NULL */
};

struct reg {
    const char *name;
    const struct reg_ops *ops; /* NULL if unassigned */
    void *user;	/* arbitrary data under the control of the register provider */
};

extern struct reg regs[NUM_REGS];


static inline uint8_t reg_read(struct reg *reg)
{
    return reg->ops->cpu_read(reg);
}


static inline void reg_write(struct reg *reg,uint8_t value)
{
    return reg->ops->cpu_write(reg,value);
}


uint8_t read_user(struct reg *reg);
void generic_reg(int n,uint8_t value,uint8_t mask);
uint8_t forbidden_read(struct reg *reg);
void forbidden_write(struct reg *reg,uint8_t value);

void init_regs(void);

#endif /* !REG_H */
