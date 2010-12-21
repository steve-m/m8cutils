/*
 * op.c - Logical and arithmetic operations            
 *
 * Written 2006 by Werner Almesberger 
 * Copyright 2006 Werner Almesberger   
 */


#include <stdint.h>
#include <stdlib.h>

#include "util.h"
#include "error.h"
#include "op.h"


/* ----- Operation handling ------------------------------------------------ */


uint32_t op_logical_or(uint32_t a,uint32_t b)
{
    return a || b;
}


uint32_t op_logical_and(uint32_t a,uint32_t b)
{
    return a && b;
}


uint32_t op_low(uint32_t a,uint32_t b)
{
    return a & 0xff;
}


uint32_t op_high(uint32_t a,uint32_t b)
{
    return a >> 8;
}


uint32_t op_or(uint32_t a,uint32_t b)
{
    return a | b;
}


uint32_t op_xor(uint32_t a,uint32_t b)
{
    return a ^ b;
}


uint32_t op_and(uint32_t a,uint32_t b)
{
    return a & b;
}


uint32_t op_eq(uint32_t a,uint32_t b)
{
    return a == b;
}


uint32_t op_ne(uint32_t a,uint32_t b)
{
    return a != b;
}


uint32_t op_lt(uint32_t a,uint32_t b)
{
    return a < b;
}


uint32_t op_gt(uint32_t a,uint32_t b)
{
    return a > b;
}


uint32_t op_le(uint32_t a,uint32_t b)
{
    return a <= b;
}


uint32_t op_ge(uint32_t a,uint32_t b)
{
    return a >= b;
}


uint32_t op_shl(uint32_t a,uint32_t b)
{
    return a << b;
}


uint32_t op_shr(uint32_t a,uint32_t b)
{
    return a >> b;
}


uint32_t op_add(uint32_t a,uint32_t b)
{
    return a+b;
}


uint32_t op_sub(uint32_t a,uint32_t b)
{
    return a-b;
}


uint32_t op_mult(uint32_t a,uint32_t b)
{
    return a*b;
}


uint32_t op_div(uint32_t a,uint32_t b)
{
    return a/b;
}


uint32_t op_mod(uint32_t a,uint32_t b)
{
    return a % b;
}


uint32_t op_logical_not(uint32_t a,uint32_t b)
{
    return !a;
}


uint32_t op_not(uint32_t a,uint32_t b)
{
    return ~a;
}


uint32_t op_minus(uint32_t a,uint32_t b)
{
    return ~a;
}


/* ----- Dummy operators --------------------------------------------------- */


uint32_t op_number(uint32_t a,uint32_t b)
{
    return 0;
}


static uint32_t op_id(uint32_t a,uint32_t b)
{
    return 0;
}


/* ----- Operation processing ---------------------------------------------- */


static struct op *new_op(uint32_t (*fn)(uint32_t a,uint32_t b))
{
    struct op *op;

    op = alloc_type(struct op);
    op->fn = fn;
    op->ref = 1;
    return op;
}


void free_op(struct op *op)
{
    if (op->fn != op_number && op->fn != op_id) {
	put_op(op->u.a);
	if (op->b)
	    put_op(op->b);
    }
    free(op);
}


struct op *number_op(uint32_t value)
{
    struct op *op;

    op = new_op(op_number);
    op->u.value = value;
    return op;
}


struct op *id_op(const struct id *id)
{
    struct op *op;

    if (id->defined)
	return get_op(id->value);
    op = new_op(op_id);
    op->u.id = id;
    return op;
}


struct op *make_op(uint32_t (*fn)(uint32_t a,uint32_t b),
  struct op *a,struct op *b)
{
    struct op *op;

    if ((!a || a->fn == op_number) && (!b || b->fn == op_number)) {
	a->u.value = fn(a->u.value,b->u.value);
	if (b)
	    free(b);
	return a;
    }
    op = new_op(fn);
    op->u.a = a;
    op->b = b;
    return op;
}


uint32_t evaluate(const struct op *op)
{
    if (op->fn == op_number)
	return op->u.value;
    if (op->fn == op_id) {
	if (!op->u.id->defined)
	    lerrorf(&op->u.id->loc,"undefined identifier \"%s\"",
	      op->u.id->name);
	return evaluate(op->u.id->value);
    }
    return op->fn(evaluate(op->u.a),op->b ? evaluate(op->b) : 0);
}
