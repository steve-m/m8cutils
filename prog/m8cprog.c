/*
 * m8cprog.c - General programmer for the M8C microcontroller family
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "interact.h"
#include "file.h"
#include "chips.h"

#include "tty.h"
#include "prog.h"
#include "ops.h"


static void decode_security(FILE *file)
{
    int i;

    fprintf(file,"Flash protection (%d blocks):\n",(int) security_size*4);
    for (i = 0; i != security_size*4; i++)
	fprintf(file,"%c%s",
	  "UFRW"[block_protection(i)],
	  (i & 63) == 63 ? "\n" : (i & 7) == 7 ? " " : "");
    if (i & 63)
	fputc('\n',file);
}


static void usage(const char *name)
{
    fprintf(stderr,
"usage: %s [-p port] [-d driver] [-v [-v]] [-q] [-3|-5] [-R] [-b|-i]\n"
"                 [-c] [-e] [-r|-w] [-z] [-s] [chip] [file]\n"
"       %s -l\n"
"       %s -V\n\n"
"  -3        set up 3V operation\n"
"  -5        set up 5V operation\n"
"  -b        use binary format (overrides auto-detection on input)\n"
"  -c        compare Flash with file, can be combined with all other\n"
"            operations\n"
"  -e        erase the Flash (this is implied by -w)\n"
"  -d driver name of programmer driver (overrides M8CPROG_DRIVER, default:\n"
"            %s)\n"
"  -i        write Intel HEX format (ignored for input; default: \"ROM\"\n"
"            format)\n"
"  -l        list supported programmers and chips\n"
"  -p port   port to programmer (overrides M8CPROG_PORT, default for tty:\n"
"            %s)\n"
"  -q        quiet operation, don't show progress bars\n"
"  -r        read Flash content from chip to file\n"
"  -s        read protection data from chip (with -c or -r: use to skip over\n"
"            inaccessible blocks and include in HEX file; with -e and -w:\n"
"            check; alone: decode and print)\n"
"  -R        run timing-critical parts at real-time priority (requires root)\n"
"  -v        verbose operation, report major events\n"
"  -v -v     more verbose operation, report vectors\n"
"  -v -v -v  very verbose operation, report communication details\n"
"  -V        only print the version number and exit\n"
"  -w        write Flash and security data from file to chip\n"
"  -z        zero read-protected blocks when reading or comparing\n"
"  chip      chip name, e.g., CY8C21323 (default: auto-detect)\n"
"  file      binary or hex file to program/verify, \"-\" for stdin/stdout\n"
"            (default: stdin/stdout)\n",
  name,name,name,programmers[0]->name,DEFAULT_TTY);
    exit(1);
}


int main(int argc,char **argv)
{
    const char *port = NULL;
    const char *driver = NULL;
    const char *file_name = "-";
    const char *chip_name = NULL;
    const struct chip *chip = NULL;
    int binary = 0,hex = 0,voltage = 0,zero = 0;
    int op_erase = 0,op_compare = 0,op_read = 0,op_write = 0,op_security = 0;
    int c,width;

    /*
     * Reserved option letters:
     * x  select XRES method even if power-on is available
     * o  pass option(s) to the driver
     */
    while ((c = getopt(argc,argv,"35bcd:eilp:qrswRvVz")) != EOF)
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
	    case 'c':
		op_compare = 1;
		break;
	    case 'd':
		driver = optarg;
		break;
	    case 'e':
		op_erase = 1;
		break;
	    case 'i':
		hex = 1;
		break;
	    case 'l':
		printf("supported programmers:\n");
		prog_list(stdout);
		printf("\nsupported chips:\n");
		width = get_output_width(stdout);
		chip_list(stdout,width ? width : DEFAULT_OUTPUT_WIDTH);
		exit(0);
	    case 'p':
		port = optarg;
		break;
	    case 'q':
		quiet = 1;
		break;
	    case 'r':
		op_read = 1;
		break;
	    case 'R':
		real_time = 1;
		break;
	    case 's':
		op_security = 1;
		break;
	    case 'w':
		op_write = 1;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'V':
		printf("m8cprog from m8cutils version %d\n",VERSION);
		exit(0);
	    case 'z':
		zero++;
		break;
	    default:
		usage(*argv);
	}

    if (binary && hex)
	usage(*argv);
    if (op_read && op_write)
	usage(*argv);
    switch (argc-optind) {
	case 0:
	    break;
	case 1:
	    if (chip_by_name(argv[optind]))
		chip_name = argv[optind];
	    else
		file_name = argv[optind];
	    break;
	case 2:
	    chip_name = argv[optind];
	    file_name = argv[optind+1];
	    break;
	default:
	    usage(*argv);
    }

    if (chip_name) {
	chip = chip_by_name(chip_name);
	if (!chip) {
	    fprintf(stderr,"chip \"%s\" is not known\n",chip_name);
	    exit(1);
	}
    }
    if (op_write || (op_compare && !op_read))
	read_file(file_name ? file_name : "-",binary);

    voltage = prog_open(port,driver,voltage);
    if (verbose)
	fprintf(stderr,"selected %dV operation\n",voltage);
    prog_initialize(op_erase || op_write,voltage);
    chip = prog_identify(chip);
    if (op_write || op_compare)
	if (program_size > chip->banks*chip->blocks*BLOCK_SIZE) {
	    fprintf(stderr,
	      "file of %lu bytes is too big for Flash of %d bytes\n",
	      (unsigned long) program_size,chip->blocks*BLOCK_SIZE);
	    exit(1);
	}
    if (op_write || op_erase)
	prog_erase(chip);
    if (op_write)
	prog_write_program(chip);
    if (op_security && !(op_write || op_erase)) {
	prog_read_security(chip);
	if (!op_read && !op_compare)
	     decode_security(stdout);
    }
    if (op_read)
	prog_read_program(chip,zero || op_security,
	  op_write || op_erase || op_security);
    if (op_compare)
	prog_compare_program(chip,zero || op_security,
	  op_write || op_erase || op_security);
    if (op_write)
	prog_write_security(chip);
    if (op_security && (op_write || op_erase))
	prog_compare_security(chip);
    if (op_read)
	write_file(file_name,binary,hex);
    return 0;
}
