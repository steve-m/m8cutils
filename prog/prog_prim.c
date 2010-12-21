/*
 * prog_prim.c - ISSP primitives
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "file.h" /* for BLOCK_SIZE */

#include "vectors.h"
#include "prog_vector.h"
#include "prog_prim.h"


struct prog_prim prog_prim;


/* ----- Default functions -----------------------------------------------0- */


static void prim_initialize_1(int power_on)
{
    if (power_on)
	prog_acquire_power_on(INIT_VECTOR);
    else
	prog_acquire_reset(INIT_VECTOR);
    prog_vectors(INITIALIZE_1_REST);
}


static void prim_initialize_2(void)
{
    prog_vectors(INITIALIZE_2);
}


static void prim_initialize_3(int voltage)
{
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


static void prim_set_bank_num(int bank)
{
    prog_vectors(SET_BANK_NUM(bank));
}


static void prim_set_block_num(int block)
{
    prog_vectors(SET_BLOCK_NUM(block));
}


static uint16_t prim_checksum(int blocks)
{
    prog_vectors(CHECKSUM_SETUP(blocks));
    return prog_vectors(READ_BYTE(KEY2) READ_BYTE(KEY1));
}


static void prim_verify(void)
{
    prog_vectors(VERIFY_SETUP);
}


static void prim_program_block(const struct chip *chip)
{
    if (chip->cy8c27xxx)
	prog_vectors(PROGRAM_BLOCK_CY8C27xxx);
    else
	prog_vectors(PROGRAM_BLOCK_REGULAR);
}


static void prim_erase_all(void)
{
    prog_vectors(ERASE);
}


static uint16_t prim_id(int raise_cpuclk)
{
    if (raise_cpuclk)
	prog_vectors(ID_SETUP);		/* AN2026 uses this */
    else
	prog_vectors(ID_SETUP_3MHz);	/* works better for the ICE */
    return prog_vectors(READ_ID_WORD);
}


static uint16_t prim_revision(void)
{
    return prog_vectors(
      READ_REG(CPU_A)
      READ_REG(CPU_X));
}


static void prim_secure(const struct chip *chip)
{
    if (chip->cy8c27xxx)
	prog_vectors(SECURE_CY8C27xxx);
    else
	prog_vectors(SECURE_REGULAR);

}


static void prim_verify_secure(void)
{
    prog_vectors(VERIFY_SECURE_SETUP);
}


static void prim_write_block(const uint8_t *data)
{
    size_t i;

    for (i = 0; i != BLOCK_SIZE; i++)
	prog_vectors(WRITE_BYTE(i,data[i]));
}


static void prim_read_block(uint8_t *data)
{
    size_t i;

    for (i = 0; i != BLOCK_SIZE; i++)
	data[i] = prog_vectors(READ_BYTE(i));
}


static uint8_t prim_return_code(void)
{
    return prog_vectors(READ_MEM(RETURN_CODE));
}


/* ----- Populate prog_prim ------------------------------------------------ */



#define COMPLETE(name) \
    if (!prog_prim.name) \
	prog_prim.name = prim_##name


void prog_prim_init(const struct prog_prim *prim,const struct prog_vector *vec)
{
    if (prim)
        prog_prim = *prim;
    if (!vec) {
        if (prog_prim.initialize_1 && prog_prim.initialize_2 &&
          prog_prim.initialize_3 && prog_prim.set_bank_num &&
          prog_prim.set_block_num && prog_prim.checksum && prog_prim.verify &&
          prog_prim.program_block && prog_prim.erase_all && prog_prim.id &&
          prog_prim.revision && prog_prim.secure && prog_prim.verify_secure &&
          prog_prim.write_block && prog_prim.read_block &&
          prog_prim.return_code)
            return;
        fprintf(stderr,"prog_prim_init: mandatory functions are missing\n");
        exit(1);
    }
    COMPLETE(initialize_1);
    COMPLETE(initialize_2);
    COMPLETE(initialize_3);
    COMPLETE(set_bank_num);
    COMPLETE(set_block_num);
    COMPLETE(checksum);
    COMPLETE(verify);
    COMPLETE(program_block);
    COMPLETE(erase_all);
    COMPLETE(id);
    COMPLETE(revision);
    COMPLETE(secure);
    COMPLETE(verify_secure);
    COMPLETE(write_block);
    COMPLETE(read_block);
    COMPLETE(return_code);
}
