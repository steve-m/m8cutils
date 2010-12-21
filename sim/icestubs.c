/*
 * icestubs.c - Replacements for functions provided by libprog
 *
 * Written 2006 by Mike Moreton
 * Copyright 2006 ZinWave
 */


#include <stdio.h>
#include <stdlib.h>
#include "prog.h"
#include "ice.h"
#include "cli.h"
#include "interact.h"


void ice_init(void)
{
	fprintf(stderr, "Invalid call to ice_init\n");
	exit(1);
}


void ice_write(int reg,uint8_t value)
{
	fprintf(stderr, "Invalid call to ice_write\n");
	exit(1);
}


uint8_t ice_read(int reg)
{
	fprintf(stderr, "Invalid call to ice_read\n");
	exit(1);
}


void prog_usage(void)
{
	/*
	 * No usage for a pseudo-ice...
	 */
}


int prog_option(char option,const char *arg)
{
    switch (option) {
	case 'v':
	    verbose++;
	    break;
	default:
	    return 0;
    }
    return 1;
}


int prog_open_cli(void)
{
	fprintf(stderr, "Invalid call to prog_open_cli\n");
	exit(1);
}


void prog_close_cli(int reuse)
{
	fprintf(stderr, "Invalid call to prog_close_cli\n");
	exit(1);
}


int prog_power_on;


void prog_initialize(int may_write,int voltage,int power_on)
{
	fprintf(stderr, "Invalid call to prog_initialize\n");
	exit(1);
}


const struct chip *prog_identify(const struct chip *chip,int raise_cpuclk)
{
	fprintf(stderr, "Invalid call to prog_identify\n");
	exit(1);
}
