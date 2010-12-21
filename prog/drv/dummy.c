/*
 * dummy.c - Dummy programmer
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "prog.h"


#ifndef BLOCK_SIZE
#define BLOCK_SIZE		64
#endif

#define BANKS			1
#define BLOCKS_PER_BANK		64


static uint16_t id = 0x19 /* ID_CY8C21323 */,revision = 0;

static uint8_t flash[BANKS][BLOCKS_PER_BANK*BLOCK_SIZE];
static uint8_t secure_flash[BANKS][BLOCK_SIZE];
static int curr_bank,curr_block;
static uint8_t return_code;
static uint8_t buffer[BLOCK_SIZE];
static const char *name;


/* ----- ISSP primitives --------------------------------------------------- */


static void dummy_initialize_1(int power_on)
{
    return_code = 0;
}


static void dummy_initialize_2(void)
{
    return_code = 0;
}


static void dummy_initialize_3(int voltage)
{
}


static void dummy_set_bank_num(int bank)
{
    curr_bank = bank & (BANKS-1);
}


static void dummy_set_block_num(int block)
{
    /* don't mirror */
    curr_block = block & (BLOCKS_PER_BANK-1);
}


static uint16_t dummy_checksum(int blocks)
{
    uint16_t sum = 0;
    int i,j;

    for (i = 0; i != BANKS; i++)
	for (j = 0; j != BLOCKS_PER_BANK*BLOCK_SIZE; j++)
	    sum += flash[i][j];
    return_code = 0;
    return sum;
}


static void dummy_verify(void)
{
    if (secure_flash[curr_bank][curr_block/4] >> (curr_block & 3)) {
	return_code = 1;
	return;
    }
    memcpy(buffer,flash[curr_bank]+curr_block*BLOCK_SIZE,BLOCK_SIZE);
    return_code = 0;
}


static void dummy_program_block(const struct chip *chip)
{
    if ((secure_flash[curr_bank][curr_block/4] >> (curr_block & 3)) > 1) {
	return_code = 1;
	return;
    }
    memcpy(flash[curr_bank]+curr_block*BLOCK_SIZE,buffer,BLOCK_SIZE);
    return_code = 0;
}


static void dummy_erase_all(void)
{
    int i;

    for (i = 0; i != BANKS; i++) {
	memset(flash[curr_bank],0,BLOCKS_PER_BANK*BLOCK_SIZE);
	memset(secure_flash[i],0,BLOCK_SIZE);
    }
    return_code = 0;
}


static uint16_t dummy_id(int raise_cpuclk)
{
    return_code = 0;
    return id;
}


static uint16_t dummy_revision(void)
{
    return_code = 0;
    return revision;
}


static void dummy_secure(const struct chip *chip)
{
    memcpy(secure_flash[curr_bank],buffer,BLOCK_SIZE);
    return_code = 0;
}


static void dummy_verify_secure(void)
{
    memcpy(buffer,secure_flash[curr_bank],BLOCK_SIZE);
    return_code = 0;
}


static void dummy_write_block(const uint8_t *data)
{
    memcpy(buffer,data,BLOCK_SIZE);
}


static void dummy_read_block(uint8_t *data)
{
    memcpy(data,buffer,BLOCK_SIZE);
}


static uint8_t dummy_return_code(void)
{
    return return_code;
}


struct prog_prim dummy_prim = {
    .initialize_1 = &dummy_initialize_1,
    .initialize_2 = &dummy_initialize_2,
    .initialize_3 = &dummy_initialize_3,
    .set_bank_num = &dummy_set_bank_num,
    .set_block_num = &dummy_set_block_num,
    .checksum = &dummy_checksum,
    .verify = &dummy_verify,
    .program_block = &dummy_program_block,
    .erase_all = &dummy_erase_all,
    .id = &dummy_id,
    .revision = &dummy_revision,
    .secure = &dummy_secure,
    .verify_secure = &dummy_verify_secure,
    .write_block = &dummy_write_block,
    .read_block = &dummy_read_block,
    .return_code = &dummy_return_code,
};


/* ----- Common ------------------------------------------------------------ */


static int dummy_open(const char *dev,int voltage,int power_on,
  const char *args[])
{
    if (args && *args) {
	FILE *file;
	int i;

	name = args[1] ? args[1] : args[0];
	file = fopen(*args,"r");
	if (file) {
	    for (i = 0; i != BANKS; i++) {
		if (fread(flash[i],1,BLOCKS_PER_BANK*BLOCK_SIZE,file) !=
		  BLOCKS_PER_BANK*BLOCK_SIZE) {
		    perror("fread");
		    exit(1);
		}
		if (fread(secure_flash[i],1,BLOCK_SIZE,file) != BLOCK_SIZE) {
		    perror("fread");
		    exit(1);
		}
	    }
	    if (fclose(file) == EOF) {
		perror("fclose");
		exit(1);
	    }
	}
    }
    prog_init(NULL,&dummy_prim,NULL,NULL);
    return voltage;
}


static void dummy_close(void)
{
    FILE *file;
    int i;

    if (!name)
	return;
    file = fopen(name,"w");
    if (!file) {
	perror("fopen");
	exit(1);
    }
    for (i = 0; i != BANKS; i++) {
	if (fwrite(flash[i],1,BLOCKS_PER_BANK*BLOCK_SIZE,file) !=
	  BLOCKS_PER_BANK*BLOCK_SIZE) {
	    perror("fwrite");
	    exit(1);
	}
	if (fwrite(secure_flash[i],1,BLOCK_SIZE,file) != BLOCK_SIZE) {
	    perror("fwrite");
	    exit(1);
	}
    }
    if (fclose(file) == EOF) {
	perror("fclose");
	exit(1);
    }
}


struct prog_common dummy = {
    .name = "dummy",
    .open = dummy_open,
    .close = dummy_close,
};
