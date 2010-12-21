/*
 * code.h - Code output
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef CODE_H
#define CODE_H

#include "op.h"


enum area_attribute {
    ATTR_RAM = 1,
    ATTR_ROM = 2,
    ATTR_ABS = 4,
    ATTR_REL = 8,
    ATTR_CON = 16,
    ATTR_OVR = 32,
};

struct area {
    char *name;
    int attributes;
    int pc;
    int highest_pc;
    struct loc loc;
};


extern int *pc;
extern int next_pc;
extern int cycles; /* cumulative CPUCLK cycles since beginning of program */
extern const struct area *text;
extern struct area *current_area;


void advance_pc(int n);
void next_statement(void);

/*
 * check_store verifies that we can perform a store operation now. store_op and
 * "store" do check_store implicitly, but you need to call it before using any
 * of the direct store functions, e.g., store_byte.
 */

void check_store(void);
void store_op(uint8_t op);
void store_byte(const struct loc *loc,int pos,uint32_t data);
void store_word(const struct loc *loc,int pos,uint32_t data);
void store_word_le(const struct loc *loc,int pos,uint32_t data);
void store_offset(const struct loc *loc,int offset,uint32_t data);

void set_base(int offset);

void store(void (*fn)(const struct loc *loc,int pos,uint32_t data),
  int offset,struct op *op);

void assertion(const struct loc *loc,struct op *op);

void resolve(void);

struct area *set_area(char *name,int attributes);

void code_init(void);
void code_cleanup(void);

#endif /* !CODE_H */
