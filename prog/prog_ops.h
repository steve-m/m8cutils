/*
 * prog_ops.h - Programming operations
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PROG_OPS_H
#define PROG_OPS_H

#include <stdio.h>

#include "chips.h"

#include "prog_prim.h"


/*
 * erase_all, compare_program, read_program, compare_security, read_security,
 * and dump_security are all optional.
 */

struct prog_ops {
    void (*initialize)(int may_write,int voltage,int power_on);
    const struct chip *(*identify)(const struct chip *chip,int raise_cpuclk);
    void (*erase_all)(const struct chip *chip);
    void (*write_program)(const struct chip *chip);
    void (*write_security)(const struct chip *chip);
    void (*compare_program)(const struct chip *chip,int zero,int use_security);
    void (*read_program)(const struct chip *chip,int zero,int use_security);
    void (*compare_security)(const struct chip *chip);
    void (*read_security)(const struct chip *chip);
    void (*dump_security)(const struct chip *chip,FILE *file);
};


extern struct prog_ops prog_ops;


int block_protection(int block);


void prog_initialize(int may_write,int voltage,int power_on);
const struct chip *prog_identify(const struct chip *chip,int raise_cpuclk);
void prog_erase_all(const struct chip *chip);
void prog_write_program(const struct chip *chip);
void prog_write_security(const struct chip *chip);
void prog_compare_program(const struct chip *chip,int zero,int use_security);
void prog_read_program(const struct chip *chip,int zero,int use_security);
void prog_compare_security(const struct chip *chip);
void prog_read_security(const struct chip *chip);
void prog_dump_security(const struct chip *chip,FILE *file);

/*
 * prog_read_security reads the security bytes into the "security" array, while
 * prog_dump_security reads and prints the entire security blocks, including
 * unused and mirrored bytes.
 */


void prog_ops_init(const struct prog_ops *ops,const struct prog_prim *prim);

#endif /* !PROG_OPS_H */
