/*
 * test.h - Generate and perform the tests
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef TEST_H
#define TEST_H

#include <stdint.h>

#include "value.h"


struct alternatives;

struct choice {
    uint64_t defined; /* pins defined by this choice and its alternatives */
    uint8_t allow[MAX_PINS];
    uint8_t external[MAX_PINS];
    struct alternatives *alternatives;
    struct choice *next;
};

struct alternatives {
    uint64_t defined; /* union of the pins defined by all choices */
    struct choice *choices;
    struct alternatives *next;
};


extern uint64_t defined;

extern struct alternatives *alternatives;
extern struct alternatives **next_alt;

extern int all; /* don't stop after the first inconsistency */
extern int do_map; /* produce a map (implies "all", but with less output) */
extern int dry_run; /* don't do any tests, just print the configurations */

int do_tests(void);

#endif /* TEST_H */
