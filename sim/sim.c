/*
 * m8csim.c - M8C simulator
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
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <readline/history.h>

#include "interact.h"
#include "symmap.h"
#include "file.h"
#include "chips.h"
#include "cpp.h"

#include "ops.h"
#include "prog.h"
#include "cli.h"
#include "ice.h"

#include "reg.h"
#include "core.h"
#include "int.h"
#include "gpio.h"
#include "id.h"
#include "sim.h"


volatile int interrupted = 0; /* SIGINT */
volatile int running = 0; /* set if we want to be interruptible */
int include_default = 1;
int saved_stdin;
const struct chip *chip;
int ice = 0;
jmp_buf error_env;


static void handle_sigint(void (*handler)(int sig))
{
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NODEFER;
    if (sigaction(SIGINT,&sa,NULL) < 0) {
	perror("sigaction");
	exit(1);
    }
}


static void sigint_handler(int sig)
{
    if (!running)
	return;
    if (interrupted) {
	handle_sigint(SIG_DFL);
	raise(SIGINT);
    }
    interrupted = 1;
}


void exception(const char *msg,...)
{
    va_list ap;

    va_start(ap,msg);
    fprintf(stderr,"EXCEPTION ");
    vfprintf(stderr,msg,ap);
    va_end(ap);
    fputc('\n',stderr);
    // exit(1);
}


static void set_program(uint8_t *pgm,size_t size)
{
    int total_rom = chip->banks*chip->blocks*BLOCK_SIZE;
    int i,n;

    if (size > total_rom) {
	fprintf(stderr,
	  "program of %lu  bytes does not fit in Flash (%d bytes)\n",
	  (unsigned long) size,total_rom);
	exit(1);
    }               
    for (i = 0; i != sizeof(rom); i++) {
	n = i % total_rom;             
	rom[i] = n < size ? pgm[n] : 0;
    }
}


static FILE *find_file(const char *name,...)
{
    va_list ap;

    if (!name || !strcmp(name,"-"))
	return stdin;
    va_start(ap,name);
    while (1) {
	FILE *file;
	const char *dir;
	char *buf;

	dir = va_arg(ap,const char *);
	if (!dir)
	    break;
	buf = malloc(strlen(name)+strlen(dir)+2);
	if (!buf) {
	    perror("malloc");
	    exit(1);
	}
	sprintf(buf,"%s/%s",dir,name);
	file = fopen(buf,"r");
	free(buf);
	if (file) {
	    return file;
	}
    }
    va_end(ap);
    fprintf(stderr,"%s: not found\n",name);
    exit(1);
}


static void usage(const char *name)
{
    fprintf(stderr,
"usage: %s [-b] [-n] [-q] [-i [programmer_option ...]] [-e interval]\n"
"              [-m symbol_file] [-I directory] [-f script] [chip] [program]\n"
"       %s -l\n"
"       %s -V\n\n"
"  -b                program is a binary (overrides auto-detection)\n"
"  -e interval       look for interrupts only every \"interval\" instructions\n"
"                    (default: 1)\n"
"  -f script         file to read the script from, \"-\" for stdin (default:\n"
"                    stdin)\n"
"  -i                use a DIY ICE\n"
"  -I directory      look for input files also in the specified directory\n"
"  -l                list supported chips\n"
"  -m symbol_file    load the symbol map from the specified file\n"
"  -n                do not include default.m8csim\n"
"  -q                quiet operation, only output the bare minimum\n"
"  -V                only print the version number and exit\n"
"programmer_option:\n"
  ,name,name,name);
    prog_usage();
    fprintf(stderr,
"  chip              chip name, e.g., CY8C21323. If using an ICE, the chip\n"
"                    is auto-detected, and the name can be omitted\n"
"  program           binary or hex file containing ROM data, \"-\" for stdin\n"
"                    (default: don't load a program)\n");
    exit(1);
}


int main(int argc,char **argv)
{
    const char *script = NULL;
    const char *program_file = NULL;
    const char *include_dir = NULL;
    const char *symbols = NULL;
    int binary = 0,voltage = 0;
    int c;

    /*
     * Reserved for future use:
     *
     * -o  pass option(s) to the driver
     * -s  time synchronization
     * -t  trace
     * -x  historical (was reserved for XRES mode)
     *
     * Available: acghjkmoruwxyz
     */
    while ((c = getopt(argc,argv,"be:f:iI:m:nqV" PROG_OPTIONS)) != EOF)
	switch (c) {
	    char *end;

	    case 'b':
		binary = 1;
		break;
	    case 'e':
		interrupt_poll_interval = strtoul(optarg,&end,0);
		if (*end)
		    usage(*argv);
		break;
	    case 'f':
		script = optarg;
		break;
	    case 'i':
		ice = 1;
		break;
	    case 'I':
		include_dir = optarg;
		break;
	    case 'm':
		symbols = optarg;
		break;
	    case 'n':
		include_default = 0;
		break;
	    case 'q':
		quiet = 1;
		break;
	    case 'V':
		printf("m8csim from m8cutils version %d\n",VERSION);
		exit(0);
	    default:
		if (!prog_option(c,optarg))
		    usage(*argv);
	}

    switch (argc-optind) {
	case 2:
	    program_file = argv[optind+1];
	    /* fall through */
	case 1:
	    chip = chip_by_name(argv[optind]);
	    if (chip)
		break;
	    if (ice && argc == optind+1) {
		program_file = argv[optind];
		break;
	    }
	    fprintf(stderr,"chip \"%s\" is not known\n",argv[optind]);
	    return 1;
	case 0:
	    if (!ice) {
		fprintf(stderr,"please specify the chip name\n");
		return 1;
	    }
	    break;
	default:
	    usage(*argv);
    }
    if ((!script || !strcmp(script,"-")) &&
      program_file && !strcmp(program_file,"-")) {
        fprintf(stderr,"script and program cannot be both read from stdin\n");
        return 1;
    }

    /*
     * We separate read_file from set_program such that the common "file not
     * found" error can happen before we try anything more complex, such as
     * talking to the programmer.
     */
    if (program_file)
	read_file(program_file ? program_file : "-",binary);
    if (symbols)
	sym_read_file_by_name(symbols);
    if (ice) {
	voltage = prog_open_cli();
	prog_initialize(0,voltage,prog_power_on);
	chip = prog_identify(chip,0);
	ice_init();
	atexit(prog_close_cli);
    }
    if (program_file)
	set_program(program,program_size);

    m8c_init();
    int_init();
    gpio_init();
    init_regs();
    id_init();

    if (include_default) {
	FILE *file;

	if (include_dir)
	    file = find_file("default.m8csim",include_dir,
	      INSTALL_PREFIX "/share/m8cutils/include",NULL);
	else
	    file = find_file("default.m8csim",
	      INSTALL_PREFIX "/share/m8cutils/include",NULL);
	saved_stdin = dup(0);
	if (saved_stdin < 0) {
	    perror("dup");
	    exit(1);
	}
	if (dup2(fileno(file),0) < 0) {
	    perror("dup2");
	    exit(1);
	}
	add_cpp_arg("-D");
	add_cpp_arg(chip->name);
#if 0 /* we don't need this yet */
	if (include_dir) {
	    add_cpp_arg("-I");
	    add_cpp_arg(include_dir);
	}
#endif
	run_cpp_on_file(NULL);
	next_file = find_file(script,".",NULL);
    }
    else {
	yyin = find_file(script,".",NULL);
	next_file = NULL;
	interactive = isatty(fileno(yyin));
    }
    if (isatty(fileno(next_file ? next_file : yyin)))
	handle_sigint(sigint_handler);
    using_history();
    while (1) {
	if (setjmp(error_env)) {
	    if (!isatty(fileno(yyin)))
		exit(1);
	    my_yyrestart();
	    yyrestart(yyin);
	}
	else {
	    (void) yyparse();
	    break;
	}
    }

    return 0;
}
