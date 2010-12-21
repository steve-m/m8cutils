/*
 * cy8c2prog.c - General programmer for the CY8C2 microcontroller family
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cy8c2prog.h"
#include "vectors.h"
#include "chips.h"
#include "file.h"
#include "tty.h"
#include "prog.h"


int verbose = 0;


static void progress(const char *label,int n,int end)
{
    int left,hash,i;

    left = OUTPUT_WIDTH-strlen(label)-1;
    hash = left*((n+0.0)/end);
    fprintf(stderr,"\r%s ",label);
    for (i = 0; i != hash; i++)
	fputc('#',stderr);
}


static void progress_clear(void)
{
    fprintf(stderr,"\r%*s\r",OUTPUT_WIDTH,"");
}


static void usage(const char *name)
{
    fprintf(stderr,
"usage: %s [-p port] [-d driver] [-v [-v]] [-3|-5] [-R] [-b]\n"
"                 (-r|-w [-c]|-c) [chip] [file]\n"
"       %s -l\n\n"
"  -3        set up 3V operation\n"
"  -5        set up 5V operation\n"
"  -b        use binary format (overrides auto-detection on input)\n"
"  -c        compare Flash with file\n"
"  -d driver name of programmer driver (overrides CY8C2PROG_DRIVER, default:\n"
"            %s)\n"
"  -l        list supported programmers and chips\n"
"  -p port   port to programmer (overrides CY8C2PROG_PORT, default for tty:\n"
"            %s)\n"
"  -r        read Flash content from chip to file\n"
//"  -R        run timing-critical parts at real-time priority (requires root)\n"
"  -v        verbose operation, report major events\n"
"  -v -v     more verbose operation, report vectors\n"
"  -v -v -v  very verbose operation, report communication details\n"
"  -w        write Flash and security data from file to chip\n"
"  chip      chip name, e.g., CY8C21323 (default: auto-detect)\n"
"  file      binary or hex file to program/verify, - for stdin/stdout\n"
"            (default: stdin/stdout)\n",
  name,name,programmers[0]->name,DEFAULT_TTY);
    exit(1);
}


int main(int argc,char **argv)
{
    const char *port = NULL;
    const char *driver = programmers[0]->name;
    const char *file_name = "-";
    const char *chip_name = NULL;
    const struct chip *chip = NULL;
    uint16_t id;
    char *env;
    int binary = 0,voltage = 0,compare = 0,do_read = 0,do_write = 0;
    int c;

    env = getenv("CY8C2PROG_PORT");
    if (env)
	port = env;
    env = getenv("CY8C2PROG_DRIVER");
    if (env)
	driver = env;

    while ((c = getopt(argc,argv,"35bcdlp:rwRv")) != EOF)
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
		compare = 1;
		break;
	    case 'd':
		driver = optarg;
		break;
	    case 'l':
		printf("supported programmers:\n");
		prog_list(stdout);
		printf("\nsupported chips:\n");
		chip_list(stdout);
		exit(0);
	    case 'p':
		port = optarg;
		break;
	    case 'r':
		do_read = 1;
		break;
	    case 'w':
		do_write = 1;
		break;
	    case 'v':
		verbose++;
		break;
	    default:
		usage(*argv);
	}

    if (!(do_read || do_write || compare))
	usage(*argv);
    if (do_read && (do_write || compare))
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
    if (do_write || compare)
	read_file(file_name ? file_name : "-",binary);

    voltage = prog_open(port,driver,voltage);
    if (verbose)
	fprintf(stderr,"selected %dV operation\n",voltage);
    prog_vectors(INITIALIZE_1);
    if (do_write) {
	prog_vectors(INITIALIZE_2);
	switch (voltage) {
	    case 3:
		prog_vectors(INITIALIZE_3_3V);
		break;
	    case 5:
		prog_vectors(INITIALIZE_3_5V);
		break;
	    default:
		abort();
	}
    }
    prog_vectors(ID_SETUP);
    id = prog_vectors(READ_ID_WORD);
    if (!chip) {
	chip = chip_by_id(id);
	if (!chip) {
	    fprintf(stderr,"chip ID 0x%04x is not known\n",id);
	    exit(1);
	}
    }
    if (chip->id != id)
	fprintf(stderr,"chip reports Id 0x%04x, expected 0x%04x\n",
	  id,chip->id);
    if (verbose)
	fprintf(stderr,"chip identified as %s (0x%04x)\n",chip->name,chip->id);
    if (do_write || compare)
	if (file_size > chip->blocks*BLOCK_SIZE) {
	    fprintf(stderr,
	      "file of %lu bytes is too big for flash of %d bytes\n",
	      (unsigned long) file_size,chip->blocks*BLOCK_SIZE);
	    exit(1);
	}
    if (do_write) {
	int block,i;
	uint16_t checksum;

	prog_vectors(ERASE);
	for (block = 0; block != file_size/BLOCK_SIZE; block++) {
	    prog_vectors(SET_BLOCK_NUM(block));
	    for (i = 0; i != BLOCK_SIZE; i++)
		prog_vectors(WRITE_BYTE(i,program[block*BLOCK_SIZE+i]));
	    if (chip->cy8c27xxx)
		prog_vectors(PROGRAM_BLOCK_CY8C27xxx);
	    else
		prog_vectors(PROGRAM_BLOCK_REGULAR);
	    progress("Write",block,file_size/BLOCK_SIZE);
	}
	progress_clear();
	prog_vectors(CHECKSUM_SETUP(chip->blocks));
	checksum =
	  prog_vectors(READ_BYTE(0xf9) READ_BYTE(0xf8));
	if (checksum != do_checksum()) {
	    fprintf(stderr,"checksum error: got 0x%04x, expected 0x%04x\n",
	      checksum,do_checksum());
	    exit(1);
	}
	if (verbose)
	    fprintf(stderr,"wrote and checksummed %lu bytes (%d blocks)\n",
	      (unsigned long) file_size,block);
    }
    if (compare) {
	int block,i;

	for (block = 0; block != file_size/BLOCK_SIZE; block++) {
	    prog_vectors(SET_BLOCK_NUM(block));
	    prog_vectors(VERIFY_SETUP);
	    for (i = 0; i != BLOCK_SIZE; i++) {
		uint8_t res;

		res = prog_vectors(READ_BYTE(i));
		if (res != program[block*BLOCK_SIZE+i]) {
		    progress_clear();
		    fprintf(stderr,
		      "comparison failed: block %d, byte %d: got 0x%02x, "
		      "expected 0x%02x\n",block,i,res,
		     program[block*BLOCK_SIZE+i]);
		    exit(1);
		}
	    }
	    progress("Compare",block,file_size/BLOCK_SIZE);
	}
	progress_clear();
	if (verbose)
	    fprintf(stderr,"compared %lu bytes (%d blocks)\n",
	      (unsigned long) file_size,block);
    }
    if (do_read) {
	int block,i;

	for (block = 0; block != chip->blocks; block++) {
	    prog_vectors(SET_BLOCK_NUM(block));
	    prog_vectors(VERIFY_SETUP);
	    for (i = 0; i != BLOCK_SIZE; i++)
		program[block*BLOCK_SIZE+i] =
		  prog_vectors(READ_BYTE(i));
	    progress("Read ",block,chip->blocks);
	}
	progress_clear();
	if (verbose)
	    fprintf(stderr,"read %d bytes (%d blocks)\n",
	      block*BLOCK_SIZE,block);
	file_size = block*BLOCK_SIZE;
	write_file(file_name,binary);
    }
    return 0;
}
