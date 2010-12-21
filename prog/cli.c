/*
 * cli.c - Command line interface, back-end
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "interact.h"
#include "chips.h"

#include "tty.h"
#include "pp.h"
#include "prog_common.h"
#include "cli.h"


int prog_voltage = 0;
const char *prog_driver = NULL;
const char *prog_port = NULL;
int real_time = 0;
int prog_power_on = 0;

static int detach = 0;
static const char **prog_options = NULL;
static int prog_options_n = 0;


void prog_usage(void)
{
    fprintf(stderr,
"  -3           set up 3V operation\n"
"  -5           set up 5V operation\n"
"  -d driver    name of programmer driver (overrides M8CPROG_DRIVER, default:\n"
"               %s)\n"
"  -D           detach from target instead of halting it\n"
"  -l           list supported programmers and chips, then exit\n"
"  -p port      port to programmer (overrides M8CPROG_PORT, default for tty:\n"
"               %s, for parallel port: %s)\n"
"  -O option    programmer-specific option\n"
"  -P           select power-on method\n"
"  -R           run timing-critical parts at real-time priority (requires "
"root)\n"
"  -v           verbose operation, report major events\n"
"  -v -v        more verbose operation, report vectors\n"
"  -v -v -v     very verbose operation, report communication details\n",
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
	case 'O':
	    prog_options_n++;
	    prog_options =
	      realloc(prog_options,sizeof(char *)*(prog_options_n+1));
	    if (!prog_options) {
		perror("realloc");
		exit(1);
	    }
	    prog_options[prog_options_n-1] = strdup(arg);
	    if (!prog_options[prog_options_n-1]) {
		perror("strdup");
		exit(1);
	    }
	    prog_options[prog_options_n] = NULL;
	    break;
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
    return prog_open(prog_port,prog_driver,prog_voltage,prog_power_on,
      prog_options);
}


void prog_close_cli(void)
{
    int i;

    prog_close(detach);
    for (i = 0; i != prog_options_n; i++)
	free((void *) prog_options[i]);
    free(prog_options);
}
