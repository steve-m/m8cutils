/*
 * prog.h - One-stop shopping header for programmer drivers
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PROG_H
#define PROG_H

#include "prog_common.h"
#include "prog_bit.h"
#include "prog_vector.h"
#include "prog_prim.h"
#include "prog_ops.h"


void prog_init(const struct prog_ops *ops,const struct prog_prim *prim,
  const struct prog_vector *vec,const struct prog_bit *bit);

#endif /* !PROG_H */
