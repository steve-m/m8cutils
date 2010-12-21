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
#include "file.h"

#include "cy8c2prog.h"
#include "vectors.h"
#include "chips.h"
#include "tty.h"
#include "prog.h"


int verbose = 0;

static int quiet = 0;


static void progress(const char *label,int n,int end)
{
    int left,hash,i;

    if (quiet)
	return;
    left = OUTPUT_WIDTH-strlen(label)-1;
    hash = left*((n+0.0)/end);
    fprintf(stderr,"\r%s ",label);
    for (i = 0; i != hash; i++)
	fputc('#',stderr);
}


static void progress_clear(void)
{
    if (!quiet)
	fprintf(stderr,"\r%*s\r",OUTPUT_WIDTH,"");
}


static int read_block(int block)
{
    uint8_t res;

    prog_vectors(SET_BLOCK_NUM(block));
    prog_vectors(VERIFY_SETUP);
    res = prog_vectors(READ_MEM(RETURN_CODE));
    switch (res) {
	case 0:
	    return 1;
	case 1:
	    return 0;
	case 2:
	    fprintf(stderr,"reset while reading block %d\n",block);
	    exit(1);
	case 3:
	    fprintf(stderr,"fatal error while reading block %d\n",block);
	    exit(1);
	default:
	    fprintf(stderr,"unknown error %d while reading block %d\n",
	      res,block);
	    exit(1);
    }
}


static void do_initialize(int may_write,int voltage)
{
    prog_vectors(INITIALIZE_1);
    if (may_write) {
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
}


static const struct chip *do_identify(const struct chip *chip)
{
    uint16_t id;

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
    if (verbose) {
	uint8_t a,x;

	a = prog_vectors(READ_REG(REG_CPU_A));
	x = prog_vectors(READ_REG(REG_CPU_X));
	fprintf(stderr,
	  "chip identified as %s (0x%04x), revision (AX) 0x%02x%02x\n",
	  chip->name,chip->id,a,x);
    }
    return chip;
}


static void do_erase(const struct chip *chip)
{
    prog_vectors(ERASE);
    if (verbose)
	fprintf(stderr,"erased the entire Flash\n");
}


static void do_write(const struct chip *chip)
{
    int block,i;
    uint16_t checksum;

    pad_file();
    for (block = 0; block != program_size/BLOCK_SIZE; block++) {
	prog_vectors(SET_BLOCK_NUM(block));
	for (i = 0; i != BLOCK_SIZE; i++)
	    prog_vectors(WRITE_BYTE(i,program[block*BLOCK_SIZE+i]));
	if (chip->cy8c27xxx)
	    prog_vectors(PROGRAM_BLOCK_CY8C27xxx);
	else
	    prog_vectors(PROGRAM_BLOCK_REGULAR);
	progress("Write",block,program_size/BLOCK_SIZE);
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
    if (security_size) {
	uint8_t res;

	if (security_size != BLOCK_SIZE) {
	    fprintf(stderr,
	      "internal error: security size is not one block\n");
	    exit(1);
	}
	for (block = 0; block != security_size/BLOCK_SIZE; block++) {
	    for (i = 0; i != BLOCK_SIZE; i++)
		prog_vectors(WRITE_BYTE(i,security[block*BLOCK_SIZE+i]));
	    if (chip->cy8c27xxx)
		prog_vectors(SECURE_CY8C27xxx);
	    else
		prog_vectors(SECURE_REGULAR);
	}
	res = prog_vectors(READ_MEM(RETURN_CODE));
	if (res) {
	    fprintf(stderr,
	      "write of security data failed with return code %d\n",res);
	    exit(1);
	}
    }
    if (verbose) {
	fprintf(stderr,
	  "wrote and checksummed %lu program bytes (%lu block%s)",
	  (unsigned long) program_size,
	  (unsigned long) program_size/BLOCK_SIZE,
	  program_size == BLOCK_SIZE ? "" : "s");
	if (security_size)
	    fprintf(stderr,
	      ",\n  wrote %lu security bytes (%lu block%s)",
	      (unsigned long) security_size,
	      (unsigned long) security_size/BLOCK_SIZE,
	      security_size == BLOCK_SIZE ? "" : "s");
	fputc('\n',stderr);
    }
}


static void do_compare(const struct chip *chip,int zero)
{
    int skipped = 0;
    int i;

    for (i = 0; i < program_size; i++) {
	int block;
	uint8_t res;

	block = i/BLOCK_SIZE;
	if (!(i & (BLOCK_SIZE-1))) {
	    int ok;

	    ok = read_block(block);
	    progress("Compare",i,program_size);
	    if (!ok) {
		if (!zero) {
		    progress_clear();
		    fprintf(stderr,"block %d is read-protected\n",block);
		    exit(1);
		}
		if (verbose > 1) {
		    progress_clear();
		    fprintf(stderr,"skipping read-protected block %d\n",block);
		}
	 	skipped++;
		i += BLOCK_SIZE-1;
		continue;
	    }
	}
	res = prog_vectors(READ_BYTE(i & (BLOCK_SIZE-1)));
	if (res != program[i]) {
	    progress_clear();
	    fprintf(stderr,
	      "comparison failed: block %d, byte %d: got 0x%02x, "
	      "expected 0x%02x\n",block,i % BLOCK_SIZE,res,
	      program[i]);
	    exit(1);
	}
    }
    progress_clear();
    if (verbose) {
	int blocks = (program_size+BLOCK_SIZE-1)/BLOCK_SIZE;

	if (skipped)
	    fprintf(stderr,"compared %lu bytes (%d block%s, %d skipped)\n",
	      (unsigned long) program_size,blocks,blocks == 1 ? "" : "s",
	      skipped );
	else
	    fprintf(stderr,"compared %lu bytes (%d block%s)\n",
	      (unsigned long) program_size,blocks,blocks == 1 ? "" : "s");
    }
}


static void do_read(const struct chip *chip,int zero)
{
    int zeroed = 0;
    int block;

    for (block = 0; block != chip->blocks; block++) {
	if (!read_block(block)) {
	    if (!zero) {
		progress_clear();
		fprintf(stderr,"block %d is read-protected\n",block);
		exit(1);
	    }
	    if (verbose > 1) {
		progress_clear();
		fprintf(stderr,"zero-filling read-protected block %d\n",block);
	    }
	    memset(program+block*BLOCK_SIZE,0,BLOCK_SIZE);
	    zeroed++;
	}
	else {
	    int i;

	    for (i = 0; i != BLOCK_SIZE; i++)
		program[block*BLOCK_SIZE+i] =
		  prog_vectors(READ_BYTE(i));
	}
	progress("Read ",block,chip->blocks);
    }
    progress_clear();
    if (verbose) {
	if (zeroed)
	    fprintf(stderr,"read %d bytes (%d blocks, %d zeroed)\n",
	      block*BLOCK_SIZE,block,zeroed);
	else
	    fprintf(stderr,"read %d bytes (%d blocks)\n",
	      block*BLOCK_SIZE,block);
    }
    program_size = block*BLOCK_SIZE;
}


static void usage(const char *name)
{
    fprintf(stderr,
"usage: %s [-p port] [-d driver] [-v [-v]] [-q] [-3|-5] [-R] [-b|-i]\n"
"                 [-c] [-e] [-r|-w] [-z] [chip] [file]\n"
"       %s -l\n\n"
"  -3        set up 3V operation\n"
"  -5        set up 5V operation\n"
"  -b        use binary format (overrides auto-detection on input)\n"
"  -c        compare Flash with file, can be combined with all other\n"
"            operations\n"
"  -e        erase the Flash (this is implied by -w)\n"
"  -d driver name of programmer driver (overrides CY8C2PROG_DRIVER, default:\n"
"            %s)\n"
"  -i        write Intel HEX format (ignored for input; default: \"ROM\"\n"
"            format)\n"
"  -l        list supported programmers and chips\n"
"  -p port   port to programmer (overrides CY8C2PROG_PORT, default for tty:\n"
"            %s)\n"
"  -q        quiet operation, don't show progress bars\n"
"  -r        read Flash content from chip to file\n"
//"  -R        run timing-critical parts at real-time priority (requires root)\n"
"  -v        verbose operation, report major events\n"
"  -v -v     more verbose operation, report vectors\n"
"  -v -v -v  very verbose operation, report communication details\n"
"  -w        write Flash and security data from file to chip\n"
"  -z        zero read-protected blocks when reading or comparing\n"
"  chip      chip name, e.g., CY8C21323 (default: auto-detect)\n"
"  file      binary or hex file to program/verify, \"-\" for stdin/stdout\n"
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
    char *env;
    int binary = 0,hex = 0,voltage = 0,zero = 0;
    int op_erase = 0,op_compare = 0,op_read = 0,op_write = 0;
    int c;

    env = getenv("CY8C2PROG_PORT");
    if (env)
	port = env;
    env = getenv("CY8C2PROG_DRIVER");
    if (env)
	driver = env;

    /*
     * Reserved option letters:
     * x  select XRES method even if power-on is available
     * o  pass option(s) to the driver
     */
    while ((c = getopt(argc,argv,"35bcdeilp:qrwRvz")) != EOF)
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
		chip_list(stdout);
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
	    case 'w':
		op_write = 1;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'z':
		zero++;
		break;
	    default:
		usage(*argv);
	}

#if 0
    if (!(op_compare || op_erase || op_read || op_write)) {
	fprintf(stderr,"must specify at least one of -c, -e, -r, or -w\n");
	exit(1);
    }
#endif
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
    if (op_write || op_compare)
	read_file(file_name ? file_name : "-",binary);

    voltage = prog_open(port,driver,voltage);
    if (verbose)
	fprintf(stderr,"selected %dV operation\n",voltage);
    do_initialize(op_erase || op_write,voltage);
    chip = do_identify(chip);
    if (op_write || op_compare)
	if (program_size > chip->blocks*BLOCK_SIZE) {
	    fprintf(stderr,
	      "file of %lu bytes is too big for Flash of %d bytes\n",
	      (unsigned long) program_size,chip->blocks*BLOCK_SIZE);
	    exit(1);
	}
    if (op_write || op_erase)
	do_erase(chip);
    if (op_write)
	do_write(chip);
    if (op_read)
	do_read(chip,zero);
    if (op_compare)
	do_compare(chip,zero);
    if (op_read)
	write_file(file_name,binary,hex);
    return 0;
}
