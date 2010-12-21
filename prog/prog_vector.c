/*
 * prog_vector.c - Vector operations
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "interact.h"

#include "vectors.h"
#include "rt.h"
#include "cli.h"
#include "prog_bit.h"
#include "prog_common.h"
#include "prog_vector.h"


struct prog_vector prog_vec;


/* ----- Time measurement -------------------------------------------------- */


void start_time(struct timeval *t0)
{
    if (gettimeofday(t0,NULL) < 0) {
	perror("gettimeofday");
	exit(1);
    }
}


/*
 * Return up to one second. Delays > 1s are indicated as 1s.
 */

int32_t delta_time_us(const struct timeval *t0)
{
    struct timeval t1;

    if (gettimeofday(&t1,NULL) < 0) {
	perror("gettimeofday");
	exit(1);
    }
    t1.tv_sec -= t0->tv_sec;
    t1.tv_usec -= t0->tv_usec;
    if (t1.tv_usec < 0) {
	t1.tv_sec--;
	t1.tv_usec += 1000000;
    }
    return t1.tv_sec ? 1000000 : t1.tv_usec;
}


/* ----- Vector processing ------------------------------------------------- */


static void wait_and_poll(void)
{
    struct timeval t0;
    int i;

    prog_bit_z();
    start_time(&t0);
    while (!prog_bit_read())
	if (delta_time_us(&t0) > 10) /* 10 us */
	    goto ready;
    /*
     * Don't try to "simplify" this loop. The way it's done makes sure that
     * we don't race between timeout and external event. If we were to take
     * the time sample _after_ reading from the port, a scheduling delay
     * added after doing the I/O could make us time out even though the
     * signal just arrived.
     *
     * The loop above is correct, though: if we've waited long enough, it's
     * always safe to proceed. (We may still have some other problem, but
     * that would be some catastrophic failure we don't try to handle
     * here.)
     */
    while (1) {
	int timeout = delta_time_us(&t0) > 100000; /* 100 ms */

	if (!prog_bit_read())
	    break;
	if (timeout) {
	    fprintf(stderr,"timed out in WAIT-AND-POLL\n");
	    exit(1);
	}
    }
ready:
    for (i = 0; i != 40; i++)
	prog_bit_send(0);
}


static uint8_t vec_vector(uint32_t v)
{
    int value = 0;
    int i;

    if (!v) {
	for (i = 0; i != 22; i++)
	    prog_bit_send(0);
    }
    else if (IS_WRITE(v)) {
	for (i = 18; i >= 0; i--)
	    prog_bit_send((v >> i) & 1);
	for (i = 0; i != 3; i++)
	    prog_bit_send(1);
    }
    else {
	for (i = 18; i >= 8; i--)
	    prog_bit_send((v >> i) & 1);
	prog_bit_z();
	prog_bit_invert();
	for (i = 0; i != 8; i++) {
	    prog_bit_z();
	    if (prog_bit_read())
		value |= 1 << (7-i);
	}
	prog_bit_z();
	prog_bit_invert();
	prog_bit_send(1);
    }
    if (IS_SSC(v))
	wait_and_poll();
    return value;
}


uint32_t do_prog_vectors(uint32_t v,...)
{
    uint32_t result = 0;
    va_list ap;

    va_start(ap,v);
    while (v != END_OF_VECTORS) {
	if (IS_WRITE(v))
	    prog_vector(v);
	else
	    result = (result << 8) | prog_vector(v);
	v = va_arg(ap,uint32_t);
    }
    va_end(ap);
    return result;
}


uint8_t prog_vector(uint32_t v)
{
    int value = 0;

    if (verbose > 1) {
	fprintf(stderr,"VECTOR 0x%08x: ",(unsigned) v);
	if (!v)
	    fprintf(stderr,"ZEROES\n");
	else {
	    if (IS_WRITE(v))
		fprintf(stderr,"WRITE %s 0x%02x,0x%02x%s\n",
		  IS_REG(v) ? "REG" : "MEM",
		  (unsigned) ((v >> 8) & 0xff),(unsigned) (v & 0xff),
		  IS_SSC(v) ? " (SSC)" : "");
	    else
		fprintf(stderr,"READ %s 0x%02x\n",
		  IS_REG(v) ? "REG" : "MEM",
		  (unsigned) ((v >> 8) & 0xff));
	}
    }
    value = prog_vec.vector(v);
    if (!IS_WRITE(v) && verbose > 1)
	fprintf(stderr,"-> 0x%02x\n",value);
    return value;
}


/* ----- Acquisition ------------------------------------------------------- */


static void do_prog_acquire(uint32_t v,int power_on,
  const char *deadline_name,int deadline)
{
    struct timeval t0,wait_sdata;
    int i;
    int32_t dt;

    if (real_time)
	realtimize();
    start_time(&t0);
    start_time(&wait_sdata);
    prog_common.initialize(power_on);
    if (power_on) {
	while (1) {
	    struct timeval t0_new;
	    int timeout;

	    timeout = delta_time_us(&wait_sdata) > 100000; /* 100 ms */
	    start_time(&t0_new);
	    if (!prog_bit_read())
		break;
	    if (timeout) {
		int missed_deadline;

		missed_deadline = delta_time_us(&t0) > deadline;
		fprintf(stderr,"power-on timed out while waiting for SDATA\n");
		if (missed_deadline)
		    fprintf(stderr,
		      "may also have missed the deadline, please try -R\n");
		exit(1);
	    }
	    t0 = t0_new;
	}
    }
    /* @@@ cheeeesy ... */
    if (prog_vec.vector != vec_vector)
	prog_vector(v);
    else {
	/*
	 * Guess what ? The first 9 bits must make the deadline, not only the
	 * first 8 !
	 */
	for (i = 18; i != 9; i--)
	    prog_bit_send((v >> i) & 1);
    }
    dt = delta_time_us(&t0);
    if (real_time)
	unrealtime();
    if (dt > deadline) {
	fprintf(stderr,"%s deadline missed: %ld > %d us\n",deadline_name,
	  (long) dt,deadline);
	exit(1);
    }
    if (prog_vec.vector == vec_vector)
	for (i = 0; i != 13; i++)
	    prog_bit_send(0);
    if (verbose > 1)
	fprintf(stderr,"acquisition vector sent in %ld/%d us\n",
	  (long) dt,deadline);
}


static uint8_t vec_acquire_reset(uint32_t v)
{
    do_prog_acquire(v,0,"Txresinit",T_XRESINIT);
    return 0;
}


void do_prog_acquire_reset(uint32_t v,uint32_t dummy)
{
    if (verbose > 1)
	fprintf(stderr,"ACQUIRE VECTOR 0x%08lx\n",(unsigned long) v);
    prog_vec.acquire_reset(v);
}


/*
 * Oddly enough, at least the CY8C21323 doesn't seem to require any of the
 * elaborate initialization AN2026A prescribes, so we just treat this like
 * reset mode.
 */

void do_prog_acquire_power_on(uint32_t v,uint32_t dummy)
{
    if (verbose > 1)
	fprintf(stderr,"ACQUIRE VECTOR 0x%08x\n",(unsigned) v);
    do_prog_acquire(v,1,"Tacq",T_ACQ);
}


/* ----- Populate prog_vector ---------------------------------------------- */


#define COMPLETE(name) \
    if (!prog_vec.name) \
	prog_vec.name = vec_##name


void prog_vector_init(const struct prog_vector *vec,const struct prog_bit *bit)
{
    if (vec)
	prog_vec = *vec;
    if (!bit) {
	if (prog_vec.acquire_reset && prog_vec.vector)
	    return;
	fprintf(stderr,"prog_vector_init: mandatory functions are missing\n");
	exit(1);
    }
    COMPLETE(acquire_reset);
    COMPLETE(vector);
}
