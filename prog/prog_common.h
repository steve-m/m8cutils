/*
 * prog_common.h - Common programmer operations and driver selection
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PROG_COMMON_H
#define PROG_COMMON_H

#include <stdio.h>

#include "prog_bit.h"
#include "prog_vector.h"
#include "prog_ops.h"


#define T_XRES		10	/* Minimum time to hold XRES, in us */
#define T_VDDWAIT	1000	/* Time to wait for VDD to stabilize, in us */

/*
 * "voltage" is 3 or 5, or 0 if unspecified. The use depends on the type of
 * programmer.
 *
 * - if the programmer neither provides nor senses Vdd, "open" must return the
 *   value in "voltage". If this value is 0, it must do so without actually
 *   accessing the programmer (since this is an error condition).
 * - if the programmer senses Vdd, "open" must fail if "voltage" is non-zero
 *   and disagrees with the sensed voltage. "open" must return the sensed
 *   voltage (3 or 5).
 * - if the programmer provides Vdd, "open" must select the specifed voltage
 *   and return the same value.
 * - if the programmer only provides Vdd if it does not sense any voltage,
 *   "open" should first follow the sense procedure above and then, if
 *   necessary, proceed with providing Vdd.
 */

/*
 * "close" should power down the programmer hardware and halt the target. The
 * target can be halted in any of the following ways:
 * - by powering it down
 * - by holding it in reset
 * - by leaving it in ISSP mode
 * This is also the order of preference for the method of halting a target.
 * The target should be left in ISSP mode if the programmer hardware is unable
 * to assert XRES after having been shut down itself.
 *
 * "detach" is like "close", but tri-states SDATA and SCLK, and sends an XRES
 * pulse. This way, the target can be tested without physically disconnecting
 * it from the programmer circuit.
 */

/*
 * "initialize", "close", and "detach" are optional.
 */

struct prog_common {
    const char *name;
    int (*open)(const char *dev,int voltage,int power_on,const char *args[]);
						/* returns voltage, 3 or 5,
						   -1 on error */
    void (*initialize)(int power_on);		/* initialize the target */
    void (*close)(void);
    void (*detach)(void);
};


extern struct prog_common *programmers[];
extern struct prog_common prog_common;


void prog_list(FILE *file);
int prog_open(const char *dev,const char *programmer,int voltage,int power_on,
  const char *args[]);
  /* "dev" and "programmer" can be NULL, to use defaults */
void prog_close(int detach);

void prog_common_init(const struct prog_common *common);

#endif /* !PROG_COMMON_H */
