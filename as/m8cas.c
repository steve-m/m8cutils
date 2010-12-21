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

#include "util.h"
#include "file.h"
#include "security.h"
#include "cpp.h"

#include "error-as.h"
#include "code.h"


#define DEFAULT_PROTECTION_FILE "flashsecurity.txt"


int yyparse(void);


static int fd0;
static char **cpp_option;
static int cpp_options = 0;


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


static void read_protection(const char *name)
{
    if (name)
	read_security(name);
    else {
	if (access(DEFAULT_PROTECTION_FILE,R_OK))
	    return;
	read_security(DEFAULT_PROTECTION_FILE);
    }
    check_security(NULL);
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
	int i;

	for (i = 0; i != cpp_options*2; i++)
	    add_cpp_arg(cpp_option[i]);
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
"usage: %s [-b|-h] [-e] [-f prot_file] [-o output_file] [-m symbol_file]\n"
"             [cpp_option ...] [file ...]\n"
"usage: %s -V\n\n"
"  -b             produce binary output (default: ROM)\n"
"  -e             enable language extensions (including CPP)\n"
"  -f prot_file   read protection from specified file (default:\n"
"                 %s)\n"
"  -h             produce Intel HEX output (default: ROM)\n"
"  -o output_file write output to the specified file (default: stdout)\n"
"  -m symbol_file write a symbol map to the specified file\n"
"  -V             only print the version number and exit\n"
"  cpp_option     -Idir, -Dname[=value], or -Uname\n"
"  file           input file(s) (default: stdin)\n",
  name,name,DEFAULT_PROTECTION_FILE);
    exit(1);
}


int main(int argc,char **argv)
{
    int binary = 0,hex = 0;
    const char *output = NULL;
    const char *symbols = NULL;
    const char *flash_security = NULL;
    int c,i;

    error_init();
    id_init();
    code_init();
    cpp_option = alloc_type_n(char *,argc*2);
    while ((c = getopt(argc,argv,"bef:hD:I:m:o:U:V")) != EOF) {
	char opt[] = "-?";

    	switch (c) {

	    case 'b':
		binary = 1;
		break;
	    case 'e':
		allow_extensions = 1;
		break;
	    case 'f':
		if (flash_security)
		    usage(*argv);
		flash_security = optarg;
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
		cpp_option[cpp_options*2] = stralloc(opt);
		cpp_option[cpp_options*2+1] = stralloc(optarg);
		cpp_options++;
		break;
	    case 'V':
		printf("m8cas from m8cutils version %s\n",VERSION);
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

    read_protection(flash_security);

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
    if (!hex)
	for (i = 0; i != security_size; i++)
	    if (security[i]) {
		fprintf(stderr,
		  "output must be Intel HEX for non-zero flash protection\n");
		exit(1);
	    }
    write_file(output ? output : "-",binary,hex);
    return 0;
}
