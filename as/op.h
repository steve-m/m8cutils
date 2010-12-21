/*
 * op.h - Logical and arithmetic operations
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef OP_H
#define OP_H

#include <stdint.h>
#include <assert.h>

#include "id.h"


struct op {
    uint32_t (*fn)(uint32_t a,uint32_t b);
    union {
	struct op *a;
	uint32_t value;
	const struct id *id;
    } u;
    struct op *b;
    int ref;
};


uint32_t op_logical_or(uint32_t a,uint32_t b);
uint32_t op_logical_and(uint32_t a,uint32_t b);
uint32_t op_low(uint32_t a,uint32_t b);
uint32_t op_high(uint32_t a,uint32_t b);
uint32_t op_or(uint32_t a,uint32_t b);
uint32_t op_xor(uint32_t a,uint32_t b);
uint32_t op_and(uint32_t a,uint32_t b);
uint32_t op_eq(uint32_t a,uint32_t b);
uint32_t op_ne(uint32_t a,uint32_t b);
uint32_t op_lt(uint32_t a,uint32_t b);
uint32_t op_gt(uint32_t a,uint32_t b);
uint32_t op_le(uint32_t a,uint32_t b);
uint32_t op_ge(uint32_t a,uint32_t b);
uint32_t op_shl(uint32_t a,uint32_t b);
uint32_t op_shr(uint32_t a,uint32_t b);
uint32_t op_add(uint32_t a,uint32_t b);
uint32_t op_sub(uint32_t a,uint32_t b);
uint32_t op_mult(uint32_t a,uint32_t b);
uint32_t op_div(uint32_t a,uint32_t b);
uint32_t op_mod(uint32_t a,uint32_t b);
uint32_t op_logical_not(uint32_t a,uint32_t b);
uint32_t op_not(uint32_t a,uint32_t b);
uint32_t op_minus(uint32_t a,uint32_t b);
uint32_t op_ctz(uint32_t a,uint32_t b);

uint32_t op_number(uint32_t a,uint32_t b); /* dummy */


static inline struct op *get_op(struct op *op)
{
    assert(op->ref);
    op->ref++;
    return op;
}


void free_op(struct op *op);


static inline void put_op(struct op *op)
{
    assert(op->ref);
    if (!--op->ref)
	free_op(op);
}


struct op *number_op(uint32_t value);
struct op *id_op(const struct id *id);
struct op *make_op(uint32_t (*fn)(uint32_t a,uint32_t b),
  struct op *a,struct op *b);
uint32_t evaluate(const struct op *op);

#endif /* !OP_H */
