/*
 * prog.c - Interface to the programmer hardware
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "interact.h"

#include "vectors.h"
#include "rt.h"
#include "cli.h"
#include "prog.h"


PROGRAMMERS_DECL

struct prog_ops *programmers[] = {
    PROGRAMMERS_OPS
    NULL
};


static struct prog_ops *prog = NULL;
static int initial = 0;


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


void prog_list(FILE *file)
{
    struct prog_ops **walk;
    int width,col = 0;

    width = get_output_width(file);
    if (!width)
	width = DEFAULT_OUTPUT_WIDTH;
    for (walk = programmers; *walk ; walk++) {
	if (strlen((*walk)->name)+col+2 >= width) {
	    putc('\n',file);
	    col = 0;
	}
	fprintf(file,"%s%s",(*walk)->name,walk[1] ? ", " : "");
	col += strlen((*walk)->name)+2;
    }
    putc('\n',file);
}


int prog_open(const char *dev,const char *programmer,int voltage,int power_on)
{
    if (!dev)
	dev = getenv("M8CPROG_PORT");
    if (!programmer)
	programmer = getenv("M8CPROG_DRIVER");
    if (!programmer)
	prog = *programmers;
    else {
	struct prog_ops **walk;

	for (walk = programmers; *walk ; walk++) {
	    prog = *walk;
	    if (!strcasecmp(prog->name,programmer))
		break;
	}
	if (!*walk) {
	    fprintf(stderr,"programmer \"%s\" is not known\n",programmer);
	    exit(1);
	}
    }
    voltage = prog->open(dev,voltage,power_on);
    if (voltage < 0) {
	fprintf(stderr,"programming mode not supported\n");
	exit(1);
    }
    initial = 1;
    return voltage;
}


static void wait_and_poll(void)
{
    struct timeval t0;
    int i;

    prog->send_z();
    start_time(&t0);
    while (!prog->read_bit())
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

	if (!prog->read_bit())
	    break;
	if (timeout) {
	    fprintf(stderr,"timed out in WAIT-AND-POLL\n");
	    exit(1);
	}
    }
ready:
    for (i = 0; i != 40; i++)
	prog->send_bit(0);
}


uint8_t prog_vector(uint32_t v)
{
    static int first = 1;
    int value = 0;
    int i;

    if (verbose > 1) {
	fprintf(stderr,"VECTOR 0x%08x: ",v);
	if (!v)
	    fprintf(stderr,"ZEROES\n");
	else {
	    if (IS_WRITE(v))
		fprintf(stderr,"WRITE %s 0x%02x,0x%02x%s\n",
		  IS_REG(v) ? "REG" : "MEM",(v >> 8) & 0xff,v & 0xff,
		  IS_SSC(v) ? " (SSC)" : "");
	    else
		fprintf(stderr,"READ %s 0x%02x\n",
		  IS_REG(v) ? "REG" : "MEM",(v >> 8) & 0xff);
	}
    }
    if (prog->vector) {
	value = prog->vector(v);
	if (!IS_WRITE(v) && verbose > 1)
	    fprintf(stderr,"-> 0x%02x\n",value);
	return value;
    }
    if (!v) {
	for (i = 0; i != 22; i++)
	    prog->send_bit(0);
    }
    else if (IS_WRITE(v)) {
	for (i = 18; i >= 0; i--)
	    prog->send_bit((v >> i) & 1);
	for (i = 0; i != 3; i++)
	    prog->send_bit(1);
	first = 0;
    }
    else {
	for (i = 18; i >= 8; i--)
	    prog->send_bit((v >> i) & 1);
	prog->send_z();
	for (i = 0; i != 8; i++) {
	    prog->send_z();
	    if (prog->read_bit())
		value |= 1 << (7-i);
	}
	prog->send_z();
	prog->send_bit(1);
	if (verbose > 1)
	    fprintf(stderr,"-> 0x%02x\n",value);
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
    if (prog->initialize)
	prog->initialize(power_on);
    if (power_on) {
	while (1) {
	    struct timeval t0_new;
	    int timeout;

	    timeout = delta_time_us(&wait_sdata) > 100000; /* 100 ms */
	    start_time(&t0_new);
	    if (!prog->read_bit())
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
    if (prog->vector)
	prog->vector(v);
    else {
	/*
	 * Guess what ? The first 9 bits must make the deadline, not only the
	 * first 8 !
	 */
	for (i = 18; i != 9; i--)
	    prog->send_bit((v >> i) & 1);
    }
    dt = delta_time_us(&t0);
    if (real_time)
	unrealtime();
    if (dt > deadline) {
	fprintf(stderr,"%s deadline missed: %ld > %d us\n",deadline_name,
	  (long) dt,deadline);
	exit(1);
    }
    if (!prog->vector)
	for (i = 0; i != 13; i++)
	    prog->send_bit(0);
    if (verbose > 1)
	fprintf(stderr,"acquisition vector sent in %ld/%d us\n",
	  (long) dt,deadline);
}


void do_prog_acquire_reset(uint32_t v,uint32_t dummy)
{
    if (verbose > 1)
	fprintf(stderr,"ACQUIRE VECTOR 0x%08x\n",v);
    if (prog->acquire_reset) {
	prog->acquire_reset(v);
	return;
    }
    do_prog_acquire(v,0,"Txresinit",T_XRESINIT);
}


/*
 * Oddly enough, at least the CY8C21323 doesn't seem to require any of the
 * elaborate initialization AN2026A prescribes, so we just treat this like
 * reset mode.
 */

void do_prog_acquire_power_on(uint32_t v,uint32_t dummy)
{
    if (verbose > 1)
	fprintf(stderr,"ACQUIRE VECTOR 0x%08x\n",v);
    do_prog_acquire(v,1,"Tacq",T_ACQ);
}


#undef OUTPUT_VECTOR
#define OUTPUT_VECTOR(v) (v)


void prog_read_block(uint8_t *data)
{
    int i;

    for (i = 0; i != BLOCK_SIZE; i++)
	data[i] = prog_vector(READ_BYTE(i));
}


void prog_write_block(const uint8_t *data)
{
    int i;

    for (i = 0; i != BLOCK_SIZE; i++)
	 prog_vector(WRITE_BYTE(i,data[i]));
}


void prog_close(int detach)
{
    if (detach) {
	if (prog->detach) {
	    prog->detach();
	    return;
	}
	fprintf(stderr,"warning: \"detach\" operation is not supported\n");
    }
    if (prog->close)
	prog->close();
}
