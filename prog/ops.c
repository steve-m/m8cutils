/*
 * ops.c - Programming operations
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "interact.h"
#include "file.h"
#include "chips.h"

#include "vectors.h"
#include "prog.h"
#include "ops.h"


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


void prog_initialize(int may_write,int voltage)
{
    if (may_write && !voltage)
	fprintf(stderr,"must specify voltage (-3 or -5)\n");
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


const struct chip *prog_identify(const struct chip *chip)
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
	fprintf(stderr,"warning: chip reports Id 0x%04x, expected 0x%04x\n",
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


void prog_erase(const struct chip *chip)
{
    prog_vectors(ERASE);
    if (verbose)
	fprintf(stderr,"erased the entire Flash\n");
}


void prog_write_program(const struct chip *chip)
{
    int block,i;
    uint16_t checksum;

    pad_file();
    for (block = 0; block != program_size/BLOCK_SIZE; block++) {
	if (chip->banks > 1 && !(block % chip->blocks))
	    prog_vectors(SET_BANK_NUM(block/chip->blocks));
	prog_vectors(SET_BLOCK_NUM(block % chip->blocks));
	for (i = 0; i != BLOCK_SIZE; i++)
	    prog_vectors(WRITE_BYTE(i,program[block*BLOCK_SIZE+i]));
	if (chip->cy8c27xxx)
	    prog_vectors(PROGRAM_BLOCK_CY8C27xxx);
	else
	    prog_vectors(PROGRAM_BLOCK_REGULAR);
	progress(stderr,"Write",block,program_size/BLOCK_SIZE);
    }
    progress_clear(stderr);
    checksum = 0;
    for (i = 0; i != chip->banks; i++) {
	if (chip->banks > 1)
	    prog_vectors(SET_BANK_NUM(i));
	prog_vectors(CHECKSUM_SETUP(chip->blocks));
	checksum += prog_vectors(READ_BYTE(0xf9) READ_BYTE(0xf8));
    }
    if (checksum != do_checksum()) {
	fprintf(stderr,"checksum error: got 0x%04x, expected 0x%04x\n",
	  checksum,do_checksum());
	exit(1);
    }
    if (verbose)
	fprintf(stderr,
	  "wrote and checksummed %lu program byte%s (%lu block%s)\n",
	  (unsigned long) program_size,program_size == 1 ? "" : "s",
	  (unsigned long) program_size/BLOCK_SIZE,
	  program_size == BLOCK_SIZE ? "" : "s");
}


void prog_write_security(const struct chip *chip)
{
    int bank;

    if (!security_size)
	return;
    for (bank = 0; bank != chip->banks; bank++) {
	int base,i;
	uint8_t res;

	if (chip->banks > 1)
	    prog_vectors(SET_BANK_NUM(bank));
	base = bank*chip->blocks/4;
	if (base >= security_size)
	    break;
	for (i = 0; i != chip->blocks/4; i++)
	    prog_vectors(WRITE_BYTE(i,
	      base+i < security_size ? security[base+i] : 0));
	if (chip->cy8c27xxx)
	    prog_vectors(SECURE_CY8C27xxx);
	else
	    prog_vectors(SECURE_REGULAR);
	res = prog_vectors(READ_MEM(RETURN_CODE));
	if (res) {
	    fprintf(stderr,
	      "write of security data failed with return code %d\n",res);
	    exit(1);
	}
    }
    if (verbose) {
	int blocks;

	blocks = chip->blocks/4;
	blocks = (security_size+blocks-1)/blocks;
	if (blocks > chip->banks)
	    blocks = chip->banks;
	fprintf(stderr,"wrote %lu security byte%s (%lu block%s)\n",
	  security_size < chip->blocks/4 ? (unsigned long) security_size :
	  chip->blocks/4,security_size == 1 ? "" : "s",
	  (unsigned long) blocks,blocks == 1 ? "" : "s");
    }
}


void prog_compare_program(const struct chip *chip,int zero)
{
    int skipped = 0;
    int i;

    for (i = 0; i < program_size; i++) {
	int block;
	uint8_t res;

	block = i/BLOCK_SIZE;
	if (!(i & (BLOCK_SIZE-1))) {
	    int ok;

	    if (chip->banks > 1 && !(block % chip->blocks))
		prog_vectors(SET_BANK_NUM(block/chip->blocks));
	    ok = read_block(block % chip->blocks);
	    progress(stderr,"Compare",i,program_size);
	    if (!ok) {
		if (!zero) {
		    progress_clear(stderr);
		    fprintf(stderr,"block %d is read-protected\n",block);
		    exit(1);
		}
		if (verbose > 1) {
		    progress_clear(stderr);
		    fprintf(stderr,"skipping read-protected block %d\n",block);
		}
	 	skipped++;
		i += BLOCK_SIZE-1;
		continue;
	    }
	}
	res = prog_vectors(READ_BYTE(i & (BLOCK_SIZE-1)));
	if (res != program[i]) {
	    progress_clear(stderr);
	    fprintf(stderr,
	      "comparison failed: block %d, byte %d: got 0x%02x, "
	      "expected 0x%02x\n",block,i % BLOCK_SIZE,res,
	      program[i]);
	    exit(1);
	}
    }
    progress_clear(stderr);
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


void prog_read_program(const struct chip *chip,int zero)
{
    int zeroed = 0;
    int block;

    for (block = 0; block != chip->blocks; block++) {
	if (chip->banks > 1 && !(block % chip->blocks))
	    prog_vectors(SET_BANK_NUM(block/chip->blocks));
	if (!read_block(block)) {
	    if (!zero) {
		progress_clear(stderr);
		fprintf(stderr,"block %d is read-protected\n",block);
		exit(1);
	    }
	    if (verbose > 1) {
		progress_clear(stderr);
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
	progress(stderr,"Read ",block,chip->blocks);
    }
    progress_clear(stderr);
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
