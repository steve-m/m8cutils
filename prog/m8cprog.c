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
#include "cli.h"


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
"usage: %s %s [-q] [-b|-i]\n"
"               [-c] [-D] [-e] [-r|-w] [-z] [-s] [chip] [file]\n"
"       %s -l\n"
"       %s -V\n\n"
"  -b        use binary format (overrides auto-detection on input)\n"
"  -c        compare Flash with file, can be combined with all other\n"
"            operations\n"
"  -e        erase the Flash (this is implied by -w)\n"
"  -i        write Intel HEX format (ignored for input; default: \"ROM\"\n"
"            format)\n"
"  -q        quiet operation, don't show progress bars\n"
"  -r        read Flash content from chip to file\n"
"  -s        read protection data from chip (with -c or -r: use to skip over\n"
"            inaccessible blocks and include in HEX file; with -e and -w:\n"
"            check; alone: decode and print)\n"
"  -V        only print the version number and exit\n"
"  -w        write Flash and security data from file to chip\n"
"  -z        zero read-protected blocks when reading or comparing\n",
  name,PROG_SYNOPSIS,name,name);
    prog_usage();
    fprintf(stderr,
"  chip      chip name, e.g., CY8C21323 (default: auto-detect)\n"
"  file      binary or hex file to program/verify, \"-\" for stdin/stdout\n"
"            (default: stdin/stdout)\n");
    exit(1);
}


int main(int argc,char **argv)
{
    const char *file_name = "-";
    const char *chip_name = NULL;
    const struct chip *chip = NULL;
    int binary = 0,hex = 0,zero = 0,voltage;
    int op_erase = 0,op_compare = 0,op_read = 0,op_write = 0,op_security = 0;
    int c;

    /*
     * Reserved option letters:
     * x  historical (was reserved for XRES mode)
     * o  pass option(s) to the driver
     */
    while ((c = getopt(argc,argv,"bceiqrswVz" PROG_OPTIONS)) != EOF)
	switch (c) {
	    case 'b':
		binary = 1;
		break;
	    case 'c':
		op_compare = 1;
		break;
	    case 'e':
		op_erase = 1;
		break;
	    case 'i':
		hex = 1;
		break;
	    case 'q':
		quiet = 1;
		break;
	    case 'r':
		op_read = 1;
		break;
	    case 's':
		op_security = 1;
		break;
	    case 'w':
		op_write = 1;
		break;
	    case 'V':
		printf("m8cprog from m8cutils version %d\n",VERSION);
		exit(0);
	    case 'z':
		zero++;
		break;
	    default:
		if (!prog_option(c,optarg))
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

    voltage = prog_open_cli();
    if (verbose)
	fprintf(stderr,"selected %dV operation, %s mode\n",voltage,
	  prog_power_on ? "power-on" : "reset");
    prog_initialize(op_erase || op_write,voltage,prog_power_on);
    chip = prog_identify(chip,1);
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
    prog_close_cli();
    return 0;
}
