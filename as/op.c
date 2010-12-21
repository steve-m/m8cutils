/*
 * op.c - Logical and arithmetic operations            
 *
 * Written 2006 by Werner Almesberger 
 * Copyright 2006 Werner Almesberger   
 */


#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
    return -a;
}


uint32_t op_ctz(uint32_t a,uint32_t b)
{
    return ffs(a)-1;
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
    op->loc = current_loc;
    return op;
}


void free_op(struct op *op)
{
    if (op->fn != op_number && op->fn != op_id) {
	put_op(op->u.a);
	if (op->u2.b)
	    put_op(op->u2.b);
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


struct op *id_op(struct id *id,int direction)
{
    struct value **val;
    struct op *op;

    val = id_resolve(id,direction);
    if (*val)
	return get_op((*val)->value);
    op = new_op(op_id);
    op->u.id = id;
    op->u2.value = val;
    return op;
}


struct op *make_op(uint32_t (*fn)(uint32_t a,uint32_t b),
  struct op *a,struct op *b)
{
    struct op *op;

    if ((!a || a->fn == op_number) && (!b || b->fn == op_number)) {
	op = number_op(fn(a->u.value,b ? b->u.value : 0));
	put_op(a);
	if (b)
	    put_op(b);
	return op;
    }
    op = new_op(fn);
    op->u.a = a;
    op->u2.b = b;
    return op;
}


uint32_t evaluate(const struct op *op)
{
    if (op->fn == op_number)
	return op->u.value;
    if (op->fn == op_id) {
	struct value **value;

	if (op->u.id->alias && op->u.id->global)
	    value = &op->u.id->alias->values;
	else
	    value = op->u2.value;
	if (*value)
	    return evaluate((*value)->value);
	lerrorf(&op->loc,"undefined label \"%s\"",op->u.id->name);
    }
    return op->fn(evaluate(op->u.a),op->u2.b ? evaluate(op->u2.b) : 0);
}
