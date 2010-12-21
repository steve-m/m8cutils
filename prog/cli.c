/*
 * cli.c - Command line interface, back-end
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>

#include "interact.h"
#include "chips.h"

#include "tty.h"
#include "pp.h"
#include "prog.h"
#include "cli.h"


int prog_voltage = 0;
const char *prog_driver = NULL;
const char *prog_port = NULL;
int real_time = 0;
int prog_power_on = 0;

static int detach = 0;


void prog_usage(void)
{
    fprintf(stderr,
"  -3        set up 3V operation\n"
"  -5        set up 5V operation\n"
"  -d driver name of programmer driver (overrides M8CPROG_DRIVER, default:\n"
"            %s)\n"
"  -D        detach from target instead of halting it\n"
"  -l        list supported programmers and chips, then exit\n"
"  -p port   port to programmer (overrides M8CPROG_PORT, default for tty:\n"
"            %s, for parallel port: %s)\n"
"  -P        select power-on method\n"
"  -R        run timing-critical parts at real-time priority (requires root)\n"
"  -v        verbose operation, report major events\n"
"  -v -v     more verbose operation, report vectors\n"
"  -v -v -v  very verbose operation, report communication details\n",
      programmers[0]->name,DEFAULT_TTY,DEFAULT_PARPORT);
}


int prog_option(char option,const char *arg)
{
    int width;

    switch (option) {
	case '3':
	    if (prog_voltage)
		return 0;
	    prog_voltage = 3;
	    break;
	case '5':
	    if (prog_voltage)
		return 0;
	    prog_voltage = 5;
	    break;
	case 'd':
	    prog_driver = arg;
	    break;
	case 'D':
	    detach = 1;
	    break;
	case 'l':
	    printf("supported programmers:\n");
	    prog_list(stdout);
	    printf("\nsupported chips:\n");
	    width = get_output_width(stdout);
	    chip_list(stdout,width ? width : DEFAULT_OUTPUT_WIDTH);
	    exit(0);
	case 'p':
	    prog_port = arg;
	    break;
	case 'P':
	    prog_power_on = 1;
	    break;
	case 'R':
	    real_time = 1;
	    break;
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
    return prog_open(prog_port,prog_driver,prog_voltage,prog_power_on);
}


void prog_close_cli(void)
{
    prog_close(detach);
}
