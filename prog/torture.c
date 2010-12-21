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

#include "interact.h"
#include "vectors.h"
#include "prog.h"
#include "ops.h"


#define ROUND_UP(n,m) (((n)+(m)-1)/(m)*(m))
#define BUFFER_SIZE ROUND_UP(BLOCK_SIZE,MD5_DIGEST_LENGTH)


static unsigned char data[BUFFER_SIZE];
static struct timeval t0;
static int cycle;


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
    fprintf(stderr,"%lu.%06lu: cycle %d",
      (unsigned long) t->tv_sec,(unsigned long) t->tv_usec,cycle);
}


static void check_block(int pass)
{
    unsigned char tmp[BUFFER_SIZE];
    int i,first,errors = 0;

    prog_read_block(tmp);
    for (i = 0; i != BLOCK_SIZE; i++)
	if (data[i] != tmp[i])
	    if (!errors++)
		first = i;
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
}


#undef OUTPUT_VECTOR
#define OUTPUT_VECTOR(v) (v)


static void do_cycle(void)
{
    unsigned char tmp[BUFFER_SIZE];
    int i;

    for (i = 0; i < BLOCK_SIZE; i += MD5_DIGEST_LENGTH)
	MD5(data+i,MD5_DIGEST_LENGTH,tmp+i);
    memcpy(data,tmp,BUFFER_SIZE);
    prog_write_block(tmp);
    prog_vector(ZERO_VECTOR); /* re-synchronize */
    check_block(1);
    prog_vector(ZERO_VECTOR);
    check_block(2);
    prog_vector(ZERO_VECTOR);
}


static void usage(const char *name)
{
    fprintf(stderr,"usage: %s [-p port] [-d driver] [-3|-5]\n",name);
    exit(1);
}


int main(int argc,char **argv)
{
    const char *port = NULL;
    const char *driver = NULL;
    const struct chip *chip;
    int voltage = 0;
    struct timeval ta;
    int c;

    while ((c = getopt(argc,argv,"35d:p:v")) != EOF)
	switch (c) {
	    case '3':
		voltage = 3;
		break;
	    case '5':
		voltage = 5;
		break;
	    case 'd':
		driver = optarg;
		break;
	    case 'p':
		port = optarg;
		break;
	    case 'v':
		verbose++;
		break;
	    default:
		usage(*argv);
	}
    voltage = prog_open(port,driver,voltage);
    prog_initialize(0,voltage);
    chip = prog_identify(NULL);
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
	do_cycle();
	cycle++;
    }
}
