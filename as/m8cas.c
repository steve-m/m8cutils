/*
 * m8cas.c - M8C assembler
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "file.h"
#include "cpp.h"

#include "error-as.h"
#include "code.h"


int yyparse(void);

static int fd0;


static void write_symbols(const char *symbols)
{
    FILE *file;

    if (!strcmp(symbols,"-"))
	file = stdout;
    else {
	file = fopen(symbols,"w");
	if (!file) {
	    perror(symbols);
	    exit(1);
	}
    }
    write_ids(file);
    /* assume "write_ids" doesn't do any other system calls */
    if (ferror(file)) {
	perror(symbols);
	exit(1);
    }
    if (file != stdout)
	if (fclose(file) == EOF) {
	    perror(symbols);
	    exit(1);
	}
}


static void do_file(char *name)
{
    if (name && !strcmp(name,"-"))
	name = NULL;
    set_file(name ? name : "<stdin>");
    if (!name)
	if (dup2(fd0,0) < 0) {
	    perror("dup2");
	    exit(1);
	}
    if (allow_extensions) {
	add_cpp_arg("-I");
	add_cpp_arg(INSTALL_PREFIX "/share/m8cutils/include");
	run_cpp_on_file(name);
    }
    else {
	int fd;

	if (name) {
	    fd = open(name,O_RDONLY);
	    if (fd < 0) {
		perror(name);
		exit(1);
	    }
	    if (dup2(fd,0) < 0) {
		perror("dup2");
		exit(1);
	    }
	}
    }
    id_begin_file();
    (void) yyparse();
    id_end_file();
    if (allow_extensions)
	reap_cpp();
}


static void usage(const char *name)
{
    fprintf(stderr,
"usage: %s [-b|-h] [-e] [-o output_file] [-m symbol_file]\n"
"             [cpp_option ...] [file ...]\n"
"usage: %s -V\n\n"
"  -b             produce binary output (default: ROM)\n"
"  -e             enable language extensions (including CPP)\n"
"  -h             produce Intel HEX output (default: ROM)\n"
"  -o output_file write output to the specified file (default: stdout)\n"
"  -m symbol_file write a symbol map to the specified file\n"
"  -V             only print the version number and exit\n"
"  cpp_option     -Idir, -Dname[=value], or -Uname\n"
"  file           input file(s) (default: stdin)\n",
  name,name);
    exit(1);
}


int main(int argc,char **argv)
{
    int cpp_options = 0,binary = 0,hex = 0;
    const char *output = NULL;
    const char *symbols = NULL;
    int c;

    error_init();
    id_init();
    code_init();
    while ((c = getopt(argc,argv,"behD:I:m:o:U:V")) != EOF) {
	char opt[] = "-?";

    	switch (c) {

	    case 'b':
		binary = 1;
		break;
	    case 'e':
		allow_extensions = 1;
		break;
	    case 'h':
		hex = 1;
		break;
	    case 'o':
		if (output)
		    usage(*argv);
		output = optarg;
		break;
	    case 'm':
		symbols = optarg;
		break;
	    case 'D':
	    case 'I':
	    case 'U':
		opt[1] = c;
		add_cpp_arg(opt);
		add_cpp_arg(optarg);
		cpp_options = 1;
		break;
	    case 'V':
		printf("m8cas from m8cutils version %d\n",VERSION);
		exit(0);
	    default:
		usage(*argv);
	}
    }
    if (binary && hex)
	usage(*argv);

    if (cpp_options && !allow_extensions) {
	fprintf(stderr,"CPP options are only supported if using CPP (-e)\n");
	return 1;
    }

    /* move stdin to a safe place, because we may open other files before */
    fd0 = dup(0);
    if (fd0 < 0) {
	perror("dup");
	exit(1);
    }
    (void) close(0);

    if (optind == argc)
	do_file(NULL);
    else {
	int i;

	for (i = optind; i != argc; i++)
	    do_file(argv[i]);
    }

    resolve();
    if (symbols)
	write_symbols(symbols);
    program_size = text->highest_pc;
    id_cleanup();
    code_cleanup();
    error_cleanup();
    write_file(output ? output : "-",binary,hex);
    return 0;
}
