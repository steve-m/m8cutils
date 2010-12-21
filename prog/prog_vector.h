/*
 * prog_vector.h - Vector operations
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PROG_VECTOR_H
#define PROG_VECTOR_H

#include <stdint.h>
#include <sys/time.h>

#include "prog_bit.h"


#define T_XRESINIT	125	/* Time to send first 8 bits (reset mode),
				   in us */
#define T_ACQ		3000	/* Time to send INITIALIZE-1 (power-op mode),
				   in us */

/*
 * It is the programmer's responsibility to detect an SSC, and to perform the
 * WAIT-AND-POLL afterwards. Zero strings after a non-SSC EXEC are explicitly
 * sent to the programmer. (We need to do this to handle the zero strings
 * inside Initialize-1.)
 */

/*
 * "acquire_reset" differs from "vector" in that it is only called for the
 * first vector of Initialize-1, and that the programmer hardware or driver
 * assumes responsibility for the correct timing. If the driver does not
 * provide "acquire_reset", prog.c will call "vector" or "send_bit" instead,
 * and compare the execution time for these calls against the Txresinit
 * deadline.
 */

struct prog_vector {
    uint8_t (*acquire_reset)(uint32_t v); /* send the first vector for
					     reset method */
    uint8_t (*vector)(uint32_t v);
};


extern struct prog_vector prog_vec;


#define END_OF_VECTORS 0xffffffff


void start_time(struct timeval *t0);
int32_t delta_time_us(const struct timeval *t0);

uint8_t prog_vector(uint32_t v);
uint32_t do_prog_vectors(uint32_t v,...);
void do_prog_acquire_reset(uint32_t v,uint32_t dummy);
void do_prog_acquire_power_on(uint32_t v,uint32_t dummy);

#define prog_acquire_reset(v) do_prog_acquire_reset(v END_OF_VECTORS)
#define prog_acquire_power_on(v) do_prog_acquire_power_on(v END_OF_VECTORS)
#define prog_vectors(v) do_prog_vectors(v END_OF_VECTORS)

void prog_vector_init(const struct prog_vector *vec,
  const struct prog_bit *bit);


#endif /* !PROG_VECTOR_H */
