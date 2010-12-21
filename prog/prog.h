/*
 * prog.h - Interface to the programmer hardware
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PROG_H
#define PROG_H

#include <stdint.h>
#include <stdio.h>

#include "file.h" /* for BLOCK_SIZE */


#define T_XRESINIT	125	/* Time to send first 8 bits (reset mode),
				   in us */
#define T_ACQ		3000	/* Time to send INITIALIZE-1 (power-op mode),
				   in us */

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

struct prog_ops {
    const char *name;
    int (*open)(const char *dev,int voltage);	/* returns voltage, 3 or 5 */
    uint8_t (*acquire_reset)(uint32_t v);	/* send the first vector for
						   reset method */
    uint8_t (*vector)(uint32_t v);		/* NULL if bit-banging */
    void (*send_bit)(int bit);			/* unused if vector-based */
    void (*send_z)(void);			/* unused if vector-based */
    int (*read_bit)(void);			/* unused if vector-based */
    void (*close)(void);			/* optional */
};

extern struct prog_ops *programmers[];


#define END_OF_VECTORS 0xffffffff


void start_time(void);
int32_t delta_time_us(void);

void prog_list(FILE *file);
int prog_open(const char *dev,const char *programmer,int voltage);
  /* "dev" and "programmer" can be NULL, to use defaults */
uint8_t prog_vector(uint32_t v);
uint32_t do_prog_vectors(uint32_t v,...);
void do_prog_acquire_reset(uint32_t v,uint32_t dummy);
void prog_read_block(uint8_t *data);
void prog_write_block(const uint8_t *data);
void prog_close(void);

#define prog_acquire_reset(v) do_prog_acquire_reset(v END_OF_VECTORS)
#define prog_vectors(v) do_prog_vectors(v END_OF_VECTORS)

#endif /* !PROG_H */
