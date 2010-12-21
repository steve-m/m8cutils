/*
 * prog_ops.c - Programming operations
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

#include "prog_common.h"
#include "prog_prim.h"
#include "prog_ops.h"


struct prog_ops prog_ops;


int block_protection(int block)
{
    /*
     * The security data can only be too short if it was all-zero. Otherwise,
     * check_security would already have corrected it.
     */
    if (block/4 > security_size)
	return 0;
    return (security[block/4] >> ((block & 3)*2)) & 3; 
}


static int read_block(int block)
{
    uint8_t res;

    prog_prim.set_block_num(block);
    prog_prim.verify();
    res = prog_prim.return_code();
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


static int checksum_ok(const struct chip *chip)
{
    int i;
    uint16_t checksum;

    checksum = 0;
    for (i = 0; i != chip->banks; i++) {
	if (chip->banks > 1)
	    prog_prim.set_bank_num(i);
	checksum += prog_prim.checksum(chip->blocks);
    }
    if (checksum == do_checksum())
	return 1;
    fprintf(stderr,"checksum error: got 0x%04x, expected 0x%04x\n",
      checksum,do_checksum());
    return 0;
}


/* ----- Default operations ------------------------------------------------ */


static void ops_initialize(int may_write,int voltage,int power_on)
{
    if (may_write && !voltage) {
	fprintf(stderr,"must specify voltage (-3 or -5)\n");
	exit(1);
    }
    prog_prim.initialize_1(power_on);
    if (may_write) {
	prog_prim.initialize_2();
	prog_prim.initialize_3(voltage);
    }
}


static const struct chip *ops_identify(const struct chip *chip,int raise_cpuclk)
{
    uint16_t id;

    id = prog_prim.id(raise_cpuclk);
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
    if (verbose)
	fprintf(stderr,
	  "chip identified as %s (0x%04x), revision (AX) 0x%04x\n",
	  chip->name,chip->id,prog_prim.revision());
    return chip;
}


static void ops_erase_all(const struct chip *chip)
{
    prog_prim.erase_all();
    if (verbose)
	fprintf(stderr,"erased the entire Flash\n");
}


static void ops_write_program(const struct chip *chip)
{
    static uint8_t zero_block[BLOCK_SIZE];
    int current_bank = 0,skipped = 0,bank,block;

    pad_file();
    for (block = 0; block != program_size/BLOCK_SIZE; block++) {
	if (!memcmp(program+block*BLOCK_SIZE,zero_block,BLOCK_SIZE))
	    skipped++;
	else {
	    if (chip->banks > 1) {
		bank = block/chip->blocks;
		if (bank != current_bank) {
		    prog_prim.set_bank_num(bank);
		    current_bank = bank;
		}
	    }
	    prog_prim.set_block_num(block % chip->blocks);
	    prog_prim.write_block(program+block*BLOCK_SIZE);
	    prog_prim.program_block(chip);
	}
	progress(stderr,"Write",block,program_size/BLOCK_SIZE);
    }
    progress_clear(stderr);
    if (!checksum_ok(chip))
	exit(1);
    if (verbose)
	fprintf(stderr,
	  "wrote and checksummed %lu program byte%s "
	  "(%lu block%s, %u skipped)\n",
	  (unsigned long) program_size,program_size == 1 ? "" : "s",
	  (unsigned long) program_size/BLOCK_SIZE,
	  program_size == BLOCK_SIZE ? "" : "s",skipped);
}


static void ops_write_security(const struct chip *chip)
{
    int bytes_per_bank,bank;
    int i;

    for (i = 0; i != security_size; i++)
	if (security[i])
	    break;
    if (i == security_size)
	return;
    bytes_per_bank = chip->blocks/4;
    for (bank = 0; bank != chip->banks; bank++) {
	uint8_t buffer[BLOCK_SIZE];
	int base;
	uint8_t res;

	if (chip->banks > 1)
	    prog_prim.set_bank_num(bank);
	base = bank*bytes_per_bank;
	/*
	 * We pad any missing bytes with 0xAA (R). Normally, check_security
	 * should catch such things, but better safe than sorry. Furthermore,
	 * we set all bytes that cannot be represented in the security data to
	 * 0xFF.
	 */
	for (i = 0; i != BLOCK_SIZE; i++)
	    buffer[i] =
	      i < bytes_per_bank || chip->banks == 1 ?
	        base+i < security_size ? security[base+i] : 0xaa :
		0xff;
	prog_prim.write_block(buffer);
	prog_prim.secure(chip);
	res = prog_prim.return_code();
	if (res) {
	    fprintf(stderr,
	      "write of security data failed with return code %d\n",res);
	    exit(1);
	}
    }
    if (verbose) {
	int blocks;

	blocks = (security_size+bytes_per_bank-1)/bytes_per_bank;
	if (blocks > chip->banks)
	    blocks = chip->banks;
	fprintf(stderr,"wrote %lu security byte%s (%lu block%s)\n",
	  security_size < bytes_per_bank ? (unsigned long) security_size :
	  bytes_per_bank,security_size == 1 ? "" : "s",
	  (unsigned long) blocks,blocks == 1 ? "" : "s");
    }
}


static void ops_compare_program(const struct chip *chip,int zero,
  int use_security)
{
    uint8_t buffer[BLOCK_SIZE];
    int skipped = 0;
    int i;

    for (i = 0; i < program_size; i++) {
	int block;
	uint8_t res;

	block = i/BLOCK_SIZE;
	if (!(i & (BLOCK_SIZE-1))) {
	    int ok = 1;

	    if (chip->banks > 1 && !(block % chip->blocks))
		prog_prim.set_bank_num(block/chip->blocks);
	    if (use_security)
		ok = !block_protection(block);
	    if (ok)
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
	    prog_prim.read_block(buffer);
	}
	res = buffer[i & (BLOCK_SIZE-1)];
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
    if (skipped && !checksum_ok(chip))
	exit(1);
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


static void ops_read_program(const struct chip *chip,int zero,int use_security)
{
    int zeroed = 0;
    int block;

    for (block = 0; block != chip->blocks; block++) {
	int ok = 1;

	if (chip->banks > 1 && !(block % chip->blocks))
	    prog_prim.set_bank_num(block/chip->blocks);
	if (use_security)
	    ok = !block_protection(block);
	if (ok)
	    ok = read_block(block);
	if (ok)
	    prog_prim.read_block(program+block*BLOCK_SIZE);
	else {
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


static void ops_compare_security(const struct chip *chip)
{
    uint8_t buffer[BLOCK_SIZE];
    int bytes,i;

    bytes = chip->blocks/4;
    for (i = 0; i != bytes; i++) {
	uint8_t expected,got;

	if (!(i & (chip->blocks-1))) {
	    uint8_t res;

	    if (chip->banks > 1)
		prog_prim.set_bank_num(i/chip->blocks);
	    prog_prim.verify_secure();
            res = prog_prim.return_code();
	    if (res) {
		fprintf(stderr,
		  "read of security data failed with return code %d\n",res);
		exit(1);
	    }
	    prog_prim.read_block(buffer);
	}
	expected = i >= security_size ? 0 : security[i];
	got = buffer[i];
	if (expected != got) {
	    fprintf(stderr,
	      "comparison failed: protection byte %d: got 0x%02x, "
	      "expected 0x%02x\n",i,got,expected);
	    exit(1);
	}
    }
    if (verbose)
	fprintf(stderr,"compared %d byte%s of security data\n",
	  bytes,bytes == 1 ? "" : "s");
}


static void ops_read_security(const struct chip *chip)
{
    uint8_t buffer[BLOCK_SIZE];
    int i,bytes_per_bank;

    bytes_per_bank = chip->blocks/4;
    security_size = chip->banks*bytes_per_bank;
    for (i = 0; i != security_size; i++) {
	if (!(i & (bytes_per_bank-1))) {
	    uint8_t res;

	    if (chip->banks > 1)
		prog_prim.set_bank_num(i/bytes_per_bank);
	    prog_prim.verify_secure();
            res = prog_prim.return_code();
	    if (res) {
		fprintf(stderr,
		  "read of security data failed with return code %d\n",res);
		exit(1);
	    }
	    prog_prim.read_block(buffer);
	}
	security[i] = buffer[i & (bytes_per_bank-1)];
    }
    if (verbose)
	fprintf(stderr,"read %d byte%s of security data\n",
	  (int) security_size,security_size == 1 ? "" : "s");
}


static void ops_dump_security(const struct chip *chip,FILE *file)
{
    uint8_t buffer[BLOCK_SIZE];
    int bank;

    for (bank = 0; bank != chip->banks; bank++) {
	uint8_t res;
	int i;

	if (chip->banks > 1)
	    prog_prim.set_bank_num(bank);
	prog_prim.verify_secure();
        res = prog_prim.return_code();
	if (res) {
	    fprintf(stderr,
	      "read of security data failed with return code %d\n",res);
	    exit(1);
	}
	prog_prim.read_block(buffer);
	for (i = 0; i != BLOCK_SIZE; i++) {
	    uint8_t byte;
	    int j;

	    byte = buffer[i];
	    if (!(i & 15) && chip->banks > 1) {
		if (i)
		    fprintf(file,"%8s","");
		else
		    fprintf(file,"Bank %d: ",bank);
	    }
	    for (j = 0; j != 4; j++)
		fputc("UFRW"[(byte >> 2*j) & 3],file);
	    if ((i & 1) == 1)
		fputc((i & 15) == 15 ? '\n' : ' ',file);
	}
    }
}



/* ----- Wrappers for calls through function pointers ---------------------- */


void prog_initialize(int may_write,int voltage,int power_on)
{
    prog_ops.initialize(may_write,voltage,power_on);
}


const struct chip *prog_identify(const struct chip *chip,int raise_cpuclk)
{
    return prog_ops.identify(chip,raise_cpuclk);
}


void prog_erase_all(const struct chip *chip)
{
    if (prog_ops.erase_all)
	prog_ops.erase_all(chip);
}


void prog_write_program(const struct chip *chip)
{
    prog_ops.write_program(chip);
}


void prog_write_security(const struct chip *chip)
{
    prog_ops.write_security(chip);
}


void prog_compare_program(const struct chip *chip,int zero,int use_security)
{
    if (prog_ops.compare_program)
	prog_ops.compare_program(chip,zero,use_security);
}


void prog_read_program(const struct chip *chip,int zero,int use_security)
{
    if (prog_ops.read_program)
	prog_ops.read_program(chip,zero,use_security);
}


void prog_compare_security(const struct chip *chip)
{
    if (prog_ops.compare_security)
	prog_ops.compare_security(chip);
}


void prog_read_security(const struct chip *chip)
{
    if (prog_ops.read_security)
	prog_ops.read_security(chip);
}


void prog_dump_security(const struct chip *chip,FILE *file)
{
    if (prog_ops.dump_security)
	prog_ops.dump_security(chip,file);
}


/* ----- Populate prog_ops ------------------------------------------------- */


#define COMPLETE(name) \
    if (!prog_ops.name) \
	prog_ops.name = ops_##name


void prog_ops_init(const struct prog_ops *ops,const struct prog_prim *prim)
{
    if (ops)
	prog_ops = *ops;
    if (!prim) {
	if (prog_ops.initialize && prog_ops.identify &&
	  prog_ops.write_program && prog_ops.write_security)
	    return;
	fprintf(stderr,"prog_ops_init: mandatory functions are missing\n");
	exit(1);
    }
    COMPLETE(initialize);
    COMPLETE(identify);
    COMPLETE(erase_all);
    COMPLETE(write_program);
    COMPLETE(write_security);
    COMPLETE(compare_program);
    COMPLETE(read_program);
    COMPLETE(compare_security);
    COMPLETE(read_security);
    COMPLETE(dump_security);
}
