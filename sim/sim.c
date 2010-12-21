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
#include <sys/types.h>

#include "interact.h"
#include "cpp.h"
#include "file.h"
#include "chips.h"

#include "ops.h"
#include "prog.h"

#include "reg.h"
#include "m8c.h"
#include "gpio.h"
#include "sim.h"


int yyparse(void);


const struct chip *chip;
int ice = 0;


void exception(const char *msg,...)
{
    va_list ap;

    va_start(ap,msg);
    fprintf(stderr,"EXCEPTION ");
    vfprintf(stderr,msg,ap);
    va_end(ap);
    fputc('\n',stderr);
    exit(1);
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


static void usage(const char *name)
{
    fprintf(stderr,
"usage: %s [-b] [-n [-n]] [-q] [-i [programmer_option ...]] [cpp_option ...]\n"
"          [-f script] chip [program]\n"
"       %s -l\n\n"
"  -b                program is a binary (overrides auto-detection)\n"
"  -f script         file to read the script from, \"-\" for stdin (default:\n"
"                    stdin)\n"
"  -i                use a DIY ICE\n"
"  -l                list supported chips\n"
"  -n                do not include default.m8csim\n"
"  -n -n             do not use CPP at all\n"
"  -q                quiet operation, only output the bare minimum\n"
"  programmer_option optional settings for the programmer used as ICE:\n"
"    -p port         port to programmer, overrides M8CPROG_PORT\n"
"    -d driver       name of programmer driver, overrides M8CPROG_DRIVER\n"
"    -3              if the programmer powers the board, supply 3V\n"
"    -5              if the programmer powers the board, supply 5V\n"
"  cpp_option        -Idir, -Dname[=value], or -Uname\n"
"  chip              chip name, e.g., CY8C21323\n"
"  program           binary or hex file containing ROM data, \"-\" for stdin\n"
"                    (default: stdin)\n"
  ,name,name);
    exit(1);
}


int main(int argc,char **argv)
{
    const char *script = NULL;
    const char *program_file = NULL;
    const char *driver = NULL;
    const char *port = NULL;
    int binary = 0,include_default = 1,voltage = 0,cpp = 1;
    int c;

    while ((c = getopt(argc,argv,"35bd:f:ilnp:qD:I:U:")) != EOF) {
        char opt[] = "-?";

	switch (c) {
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
	    case 'f':
		script = optarg;
		break;
	    case 'i':
		ice = 1;
		break;
	    case 'l':
		chip_list(stdout,79);
		return 0;
	    case 'n':
		if (include_default)
		    include_default = 0;
		else
		    cpp = 0;
		break;
	    case 'p':
		port = optarg;
		break;
	    case 'q':
		quiet = 1;
		break;
	    case 'D':
	    case 'I':
	    case 'U':
		opt[1] = c;
		add_cpp_arg(opt);
		add_cpp_arg(optarg);
		break;
	    default:
		usage(*argv);

	}
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
    }

    m8c_init();
    gpio_init();
    init_regs();

    if (cpp) {
	if (include_default) {
	    add_cpp_arg("-include");
	    add_cpp_arg("default.m8csim");
	}
	run_cpp_on_file(script);
    }
    else {
	int fd;

	if (script && strcmp(script,"-")) {
	    fd = open(script,O_RDONLY);
	    if (fd < 0) {
		perror(script);
		exit(1);
	    }
	    if (dup2(fd,0) < 0) {
		perror("dup2");
		exit(1);
	    }
	}
    }
    (void) yyparse();
    if (ice)
	prog_close();
    if (cpp)
	reap_cpp();
    return 0;
}
