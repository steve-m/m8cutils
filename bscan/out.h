/*
 * out.h - Produce human-readable output
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef OUT_H
#define OUT_H

#include <stdint.h>


extern int patterns; /* number of distinct setups tried */
extern int tests; /* number of single-bit tests */


void current(int bit,int value);
void status(void);
void print_dry_run(uint64_t set);
int expect(int bit,int want,int got,int current_bit,int current_value);
void lost_comm(int current_bit,int current_value);
int finish(void);

#endif /* OUT_H */
