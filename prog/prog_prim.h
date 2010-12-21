/*
 * prog_prim.h - ISSP primitives
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PROG_PRIM_H
#define PROG_PRIM_H


#include <stdint.h>
#include <sys/types.h>

#include "chips.h"

#include "prog_vector.h"


struct prog_prim {
    void (*initialize_1)(int power_on);
    void (*initialize_2)(void);
    void (*initialize_3)(int voltage);
    void (*set_bank_num)(int bank);
    void (*set_block_num)(int block);
    uint16_t (*checksum)(int blocks);
    void (*verify)(void);
    void (*program_block)(const struct chip *chip);
    void (*erase_all)(void);
    uint16_t (*id)(int raise_cpuclk);
    uint16_t (*revision)(void);
    void (*secure)(const struct chip *chip);
    void (*verify_secure)(void);
    void (*write_block)(const uint8_t *data);
    void (*read_block)(uint8_t *data);
    uint8_t (*return_code)(void);
};


extern struct prog_prim prog_prim;


void prog_prim_init(const struct prog_prim *prim,const struct prog_vector *vec);

#endif /* !PROG_PRIM_H */
