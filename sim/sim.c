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
#include "file.h"
#include "chips.h"

#include "ops.h"
#include "prog.h"

#include "reg.h"
#include "core.h"
#include "int.h"
#include "gpio.h"
#include "id.h"
#include "sim.h"


volatile int interrupted = 0; /* SIGINT */
volatile int running = 0; /* set if we want to be interruptible */
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
"              [-I directory] [-f script] chip [program]\n"
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
"  -n                do not include default.m8csim\n"
"  -q                quiet operation, only output the bare minimum\n"
"  programmer_option optional settings for the programmer used as ICE:\n"
"    -p port         port to programmer, overrides M8CPROG_PORT\n"
"    -d driver       name of programmer driver, overrides M8CPROG_DRIVER\n"
"    -3              if the programmer powers the board, supply 3V\n"
"    -5              if the programmer powers the board, supply 5V\n"
"    -v ...          verbose operation\n"
"  -V                only print the version number and exit\n"
"  chip              chip name, e.g., CY8C21323\n"
"  program           binary or hex file containing ROM data, \"-\" for stdin\n"
"                    (default: stdin)\n"
  ,name,name,name);
    exit(1);
}


int main(int argc,char **argv)
{
    const char *script = NULL;
    const char *program_file = NULL;
    const char *driver = NULL;
    const char *port = NULL;
    const char *include_dir = NULL;
    int binary = 0,include_default = 1,voltage = 0;
    int c;

    /*
     * Reserved for future use:
     *
     * -s  time synchronization
     * -t  trace
     *
     * Available: acghjkmoruwxyz
     */
    while ((c = getopt(argc,argv,"35bd:e:f:iI:lnp:qvV")) != EOF)
	switch (c) {
	    char *end;

	    case '3':
		if (voltage)
		    usage(*argv);
		voltage = 3;
		break;
	    case '5':
		if (voltage)
		    usage(*argv);
		voltage = 5;
		break;
	    case 'b':
		binary = 1;
		break;
	    case 'd':
		driver = optarg;
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
	    case 'l':
		chip_list(stdout,79);
		return 0;
	    case 'n':
		include_default = 0;
		break;
	    case 'p':
		port = optarg;
		break;
	    case 'q':
		quiet = 1;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'V':
		printf("m8csim from m8cutils version %d\n",VERSION);
		exit(0);
	    default:
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
	    fprintf(stderr,"chip \"%s\" is not known\n",argv[optind]);
	    return 1;
	default:
	    usage(*argv);
    }
    if ((!script || !strcmp(script,"-")) &&
      (!program_file || !strcmp(program_file,"-"))) {
	fprintf(stderr,"script and program cannot be both read from stdin\n");
	return 1;
    }

    read_file(program_file ? program_file : "-",binary);
    set_program(program,program_size);

    if (ice) {
	voltage = prog_open(port,driver,voltage);
	prog_initialize(0,voltage);
	chip = prog_identify(chip);
	atexit(prog_close);
    }

    m8c_init();
    int_init();
    gpio_init();
    init_regs();
    id_init();

    if (include_default) {
	yyin = find_file("default.m8csim",
	  INSTALL_PREFIX "/share/m8cutils/include",include_dir,NULL);
	next_file = find_file(script,".",NULL);
    }
    else {
	yyin = find_file(script,".",NULL);
	next_file = NULL;
    }
    interactive = isatty(fileno(yyin));
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
