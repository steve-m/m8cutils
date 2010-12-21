/*
 * prog.c - One-stop shopping header for programmer drivers
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include "prog_common.h"
#include "prog_bit.h"
#include "prog_vector.h"
#include "prog_prim.h"
#include "prog_ops.h"
#include "prog.h"


void prog_init(const struct prog_ops *ops,const struct prog_prim *prim,
  const struct prog_vector *vec,const struct prog_bit *bit)
{
    if (bit) {
	prog_bit_init(bit);
	bit = &prog_bit;
    }
    if (vec || bit) {
	prog_vector_init(vec,bit);
	vec = &prog_vec;
    }
    if (prim || vec) {
	prog_prim_init(prim,vec);
	prim = &prog_prim;
    }
    prog_ops_init(ops,prim);
}
