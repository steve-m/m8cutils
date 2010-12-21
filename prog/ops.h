/*
 * ops.c - Programming operations
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef OPS_H
#define OPS_H

#include "chips.h"

void prog_initialize(int may_write,int voltage);
const struct chip *prog_identify(const struct chip *chip);
void prog_erase(const struct chip *chip);
void prog_write_program(const struct chip *chip);
void prog_write_security(const struct chip *chip);
void prog_compare_program(const struct chip *chip,int zero);
void prog_read_program(const struct chip *chip,int zero);

#endif /* !OPS_H */
