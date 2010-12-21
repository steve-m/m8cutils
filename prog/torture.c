/*
 * torture.c - Torture-test a programmer
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <openssl/md5.h>

#include "file.h" /* for BLOCK_SIZE */

#include "interact.h"
#include "vectors.h"
#include "prog_vector.h"
#include "prog_prim.h"
#include "prog_ops.h"
#include "cli.h"


#define ROUND_UP(n,m) (((n)+(m)-1)/(m)*(m))
#define BUFFER_SIZE ROUND_UP(BLOCK_SIZE,MD5_DIGEST_LENGTH)


static uint8_t data[BUFFER_SIZE];
static struct timeval t0;
static int session = 0,cycle = 0,blocks_in_session = 0;
static int connected = 0;
static int can_resynchronize;


/* ----- Timekeeping ------------------------------------------------------- */


static void get_time(struct timeval *now)
{
    if (gettimeofday(now,NULL) < 0) {
	perror("gettimeofday");
	exit(1);
    }
}


static void delta_t(struct timeval *res,const struct timeval *a,
  const struct timeval *b)
{
    res->tv_sec = a->tv_sec-b->tv_sec;
    res->tv_usec = a->tv_usec-b->tv_usec;
    if (res->tv_usec < 0) {
	res->tv_sec--;
	res->tv_usec += 1000000;
    }
}


static void now(struct timeval *t)
{
    delta_t(t,t,&t0);
    fprintf(stderr,"%lu.%06lu: session %d cycle %d",
      (unsigned long) t->tv_sec,(unsigned long) t->tv_usec,session,cycle);
}


#undef OUTPUT_VECTOR
#define OUTPUT_VECTOR(v) (v)


/* ----- Torture testing --------------------------------------------------- */


static int check_block(int pass)
{
    uint8_t tmp[BUFFER_SIZE];
    int i,first,errors = 0,zero = 0;

    prog_prim.read_block(tmp);
    for (i = 0; i != BLOCK_SIZE; i++)
	if (data[i] == tmp[i])
	    zero = 0;
	else {
	    if (!errors++)
		first = i;
	    if (!data[i] || tmp[i])
		zero = 0;
	    else {
		zero++;
		if (zero == 3)
		    return 1;
	    }
	}
    if (errors) {
	struct timeval t;

	get_time(&t);
	now(&t);
	fprintf(stderr,", pass %d, byte %d: wrote 0x%02x read 0x%02x",
	  pass,first,data[first],tmp[first]);
	if (errors == 1)
	    fputc('\n',stderr);
	else
	    fprintf(stderr," (%d more)\n",errors-1);
    }
    return 0;
}


/*
 * Try to re-synchronize in case we had some SCLK noise.
 */

static void resynchronize(void)
{
    if (can_resynchronize)
	prog_vector(ZERO_VECTOR);
}


static int do_block(void)
{
    uint8_t tmp[BUFFER_SIZE];
    int i;

    for (i = 0; i < BLOCK_SIZE; i += MD5_DIGEST_LENGTH)
	MD5(data+i,MD5_DIGEST_LENGTH,tmp+i);
    memcpy(data,tmp,BUFFER_SIZE);
    prog_prim.write_block(tmp);
    resynchronize();
    if (check_block(1))
	return 1;
    resynchronize();
    if (check_block(2))
	return 1;
    resynchronize();
    return 0;
}


static void new_session(void)
{
    int voltage = 0;

    if (connected)
	prog_close_cli(1);
    voltage = prog_open_cli();
    prog_initialize(0,voltage,prog_power_on);
    prog_identify(NULL,0);
    connected = 1;
    session++;
    blocks_in_session = 0;
}


static void do_cycle(int blocks)
{
    if (!connected || (blocks && blocks_in_session == blocks))
	new_session();
    if (do_block()) {
	struct timeval t;

	get_time(&t);
	now(&t);
	fprintf(stderr,", session lost\n");
	new_session();
    }
}


static void torture_test(int blocks)
{
    struct timeval ta;

    can_resynchronize = !!prog_vec.vector;
	
    get_time(&t0);
    ta = t0;
    while (1) {
	struct timeval tb,dt;

	get_time(&tb);
	delta_t(&dt,&tb,&ta);
	if (dt.tv_sec) {
	    ta = tb;
	    now(&tb);
	    fputc('\r',stderr);
	    fflush(stderr);
	}
	do_cycle(blocks);
	cycle++;
	blocks_in_session++;
    }
}


/* ----- Throughput measurements ------------------------------------------- */


static void speed(int blocks,const struct timeval *dt)
{
    if (!dt->tv_sec && !dt->tv_usec) {
	fprintf(stderr,"measurement too short\n");
	exit(1);
    }
    printf(": %d blocks in %lu.%06lu s = %d bytes/sec\n",
      blocks,(unsigned long) dt->tv_sec,(unsigned long) dt->tv_usec,
      (int) (blocks*BLOCK_SIZE/(dt->tv_sec+dt->tv_usec/1000000.0)+0.5));
}


static void measure(int blocks,uint8_t pattern)
{
    struct timeval t1,dt;
    uint8_t in[BLOCK_SIZE],out[BLOCK_SIZE];
    int i;

    memset(out,pattern,BLOCK_SIZE);

    get_time(&t0);
    for (i = 0; i != blocks; i++)
	prog_prim.write_block(out);
    get_time(&t1);
    delta_t(&dt,&t1,&t0);
    printf("Pattern 0x%02x: write",pattern);
    speed(blocks,&dt);

    get_time(&t0);
    for (i = 0; i != blocks; i++)
	prog_prim.read_block(in);
    get_time(&t1);

    /*
     * Verifying the data we get back is hardly worth the effort, since it's
     * overwritten lots of times, but at least we will catch some errors, and
     * really badly broken systems.
     */
     
    if (memcmp(in,out,BLOCK_SIZE)) {
	fprintf(stderr,"read/write mismatch detected\n");
	exit(1);
    }

    delta_t(&dt,&t1,&t0);
    printf("              read ");
    speed(blocks,&dt);
}


static void measure_throughput(int blocks)
{
    int voltage = 0;

    if (connected)
	prog_close_cli(1);
    voltage = prog_open_cli();
    prog_initialize(0,voltage,prog_power_on);
    prog_identify(NULL,0);

    measure(blocks,0);
    measure(blocks,0xff);
    measure(blocks,0x55);

    prog_close_cli(0);
}


/* ----- Invocation -------------------------------------------------------- */


static void usage(const char *name)
{
    fprintf(stderr,"usage: %s [programmer_option ...] [blocks]\n",name);
    fprintf(stderr,"       %s [programmer_option ...] -t blocks\n\n",name);
    fprintf(stderr,"  -t           test communication throughput\n");
    fprintf(stderr,"programmer option:\n");
    prog_usage();
    exit(1);
}


int main(int argc,char **argv)
{
    int throughput = 0;
    char *end;
    int blocks = 0;
    int c;

    while ((c = getopt(argc,argv,"t" PROG_OPTIONS)) != EOF)
	switch (c) {
	    case 't':
		throughput = 1;
		break;
	    default:
		if (!prog_option(c,optarg))
			usage(*argv);
	}
    switch (argc-optind) {
	case 0:
	    if (throughput)
		usage(*argv);
	    blocks = 0;
	    break;
	case 1:
	    blocks = strtoul(argv[optind],&end,0);
	    if (*end)
		usage(*argv);
	    break;
	default:
	    usage(*argv);
    }

    if (throughput)
	measure_throughput(blocks);
    else
	torture_test(blocks);
    return 0;
}
