/*
 * bscan.c - Boundary scanner
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cpp.h"
#include "error.h"
#include "prog.h"
#include "cli.h"
#include "ice.h"
#include "interact.h"

#include "test.h"


int yyparse(void);


static void do_file(char *name)
{
    if (name && !strcmp(name,"-"))
	name = NULL;
    set_file(name ? name : "<stdin>");
    run_cpp_on_file(name);
    (void) yyparse();
    reap_cpp();
}


static void close_cli(void)
{
    prog_close_cli(0);
}


static void usage(const char *name)
{
    fprintf(stderr,
"usage: %s [-a] [-m] [-q] [-t] [-v] programmer_option ... [cpp_option ...]\n"
"       %7s [setup_file]\n"
"usage: %s -V\n\n"
"  -a             look for all errors, not only the first one\n"
"  -m             produce a map with the test results\n"
"  -q             suppress progress indications\n"
"  -t             print what tests would be performed, without accessing the\n"
"                 target\n"
"  -v             verbose operation\n"
"  -V             only print the version number and exit\n"
"  cpp_option     -Idir, -Dname[=value], or -Uname\n"
"  programmer_option:\n"
  ,name,"",name);
    fprintf(stderr,
"  setup_file     system description file(s) (default: stdin)\n");
    exit(1);
}


int main(int argc,char **argv)
{
    int c;

    error_init();
    while ((c = getopt(argc,argv,"amqtvD:I:U:V" PROG_OPTIONS)) != EOF) {
	char opt[] = "-?";

    	switch (c) {
	    case 'a':
		all = 1;
		break;
	    case 'm':
		do_map = 1;
		break;
	    case 'q':
		quiet = 1;
		break;
	    case 't':
		dry_run = 1;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'D':
	    case 'I':
	    case 'U':
		opt[1] = c;
		add_cpp_arg(opt);
		add_cpp_arg(optarg);
		break;
	    case 'V':
		printf("m8cbscan from m8cutils version %s\n",VERSION);
		exit(0);
	    default:
		if (!prog_option(c,optarg))
		    usage(*argv);
	}
    }

    switch (argc-optind) {
	case 0:
	    do_file(NULL);
	    break;
	case 1:
	    do_file(argv[optind]);
	    break;
	default:
	    usage(*argv);
    }

    if (!dry_run) {
	int voltage;

	voltage = prog_open_cli();
	prog_initialize(0,voltage,prog_power_on);
	prog_identify(NULL,0);
	atexit(close_cli);
    }

    return do_tests();
}
