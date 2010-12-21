/*
 * vectors.h - Vector definitions
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

#ifndef VECTORS_H
#define VECTORS_H

/*
 * Note: this file is intended to be useful also for inclusion into non-C
 * programs, so we use only macros, even though enums or inline functions would
 * often be more elegant.
 */


#include "m8c.h"
#include "ssc.h"

/*
 * We encode vectors in 24 bits as follows:
 *
 * 0xSCAADD
 *
 * S = SSC indicator, 0 or 1. The programmer must wait-and-poll after executing
 *     an SSC.
 * C = Programmer operation code, see PROG_OP_* below
 * A = Address, one byte
 * D = Data, one byte. Undefined if operation is a read.
 */


#define PROG_SSC_FLAG		0x100000

#define PROG_OP_WRITE_MEM	4
#define PROG_OP_WRITE_REG	6
#define PROG_OP_READ_MEM	5
#define PROG_OP_READ_REG	7

/*
 * #define OUTPUT_VECTOR before #including this file to use a different output
 * format.
 */

#ifndef OUTPUT_VECTOR
#define OUTPUT_VECTOR(v) (v),
#endif

#define VECTOR_VALUE(c,a,d) \
  (((c) << 16) | ((a) << 8) | (d))
#define VECTOR(c,a,d)	OUTPUT_VECTOR(VECTOR_VALUE(c,a,d))

#define READ_REG(a)	VECTOR(PROG_OP_READ_REG,a,0)
#define WRITE_REG(a,d)	VECTOR(PROG_OP_WRITE_REG,a,d)
#define READ_MEM(a)	VECTOR(PROG_OP_READ_MEM,a,0)
#define WRITE_MEM(a,d)	VECTOR(PROG_OP_WRITE_MEM,a,d)

#define	ZERO_VECTOR	VECTOR(0,0,0)
#define	INIT_VECTOR	WRITE_REG(0x50,0)

#define IS_REG(v)	((!!((v) & 0x20000)))
#define IS_WRITE(v)	(!(v) || (!((v) & 0x10000)))
#define IS_EXEC(v)	((v) == EXEC_VECTOR_VALUE)
#define IS_SSC(v)	((v) == EXEC_VECTOR_SSC_VALUE)

#define MAGIC_EXEC	0x12	/* magic number to trigger execution */

#define EXEC_VECTOR_VALUE \
  VECTOR_VALUE(PROG_OP_WRITE_REG,CPU_SCR0,MAGIC_EXEC)
#define EXEC_VECTOR_SSC_VALUE  \
  VECTOR_VALUE(PROG_OP_WRITE_REG | (PROG_SSC_FLAG >> 16),CPU_SCR0,MAGIC_EXEC)
#define EXEC_VECTOR	OUTPUT_VECTOR(EXEC_VECTOR_VALUE)
#define EXEC_VECTOR_SSC	OUTPUT_VECTOR(EXEC_VECTOR_SSC_VALUE)

#define OP_SSC			0x00
#define OP_HALT			0x30
#define OP_NOP			0x40
#define OP_MOV_A_SD_MEM		0x51	/* MOV A,[expr] */
#define OP_MOV_A_DD_REG		0x60	/* MOV reg[expr],A */

#define VECTOR_PTROP_PREAMBLE(pointer) \
  WRITE_REG(CPU_F,0) \
  WRITE_REG(CPU_SP,0) \
  WRITE_MEM(KEY1,KEY1_MAGIC) \
  WRITE_MEM(KEY2,3) \
  WRITE_REG(CPU_PCH,0) \
  WRITE_REG(CPU_PCL,3) \
  WRITE_MEM(POINTER,pointer) \
  WRITE_REG(CPU_CODE1,OP_HALT) \
  WRITE_REG(CPU_CODE2,OP_NOP) \

#define VECTOR_SSC(code) \
  WRITE_REG(CPU_A,code) \
  WRITE_REG(CPU_CODE0,OP_SSC) \
  EXEC_VECTOR_SSC

#define INITIALIZE_1_REST \
  ZERO_VECTOR \
  ZERO_VECTOR \
  ZERO_VECTOR \
  ZERO_VECTOR \
  ZERO_VECTOR \
  VECTOR_PTROP_PREAMBLE(0x80) \
  VECTOR_SSC(SSC_Calibrate1)

#define INITIALIZE_1 \
  INIT_VECTOR \
  INITIALIZE_1_REST

#define INITIALIZE_2 \
  VECTOR_PTROP_PREAMBLE(0x80) \
  WRITE_MEM(BLOCKID,1) \
  VECTOR_SSC(SSC_TableRead)

#define VECTOR_MOV_A_SD_MEM(addr) \
  WRITE_REG(CPU_F,0) \
  WRITE_REG(CPU_PCL,3) \
  WRITE_REG(CPU_PCH,0) \
  WRITE_REG(CPU_SP,8) \
  WRITE_REG(CPU_CODE0,OP_MOV_A_SD_MEM) \
  WRITE_REG(CPU_CODE1,addr) \
  WRITE_REG(CPU_CODE2,OP_HALT) \
  EXEC_VECTOR \
  ZERO_VECTOR

#define VECTOR_MOV_A_DD_REG_1(addr) \
  WRITE_REG(CPU_F,0) \
  WRITE_REG(CPU_PCL,3) \
  WRITE_REG(CPU_PCH,0) \
  WRITE_REG(CPU_SP,8) \
  WRITE_REG(CPU_CODE0,OP_MOV_A_DD_REG) \
  WRITE_REG(CPU_CODE1,(addr) & 0xff) \
  WRITE_REG(CPU_CODE2,OP_HALT) \
  WRITE_REG(CPU_F,CPU_F_XIO) \
  EXEC_VECTOR \
  ZERO_VECTOR

#define INITIALIZE_3(imo,bdg) \
  VECTOR_MOV_A_SD_MEM(imo) \
  VECTOR_MOV_A_DD_REG_1(BDG_TR) \
  VECTOR_MOV_A_SD_MEM(bdg) \
  VECTOR_MOV_A_DD_REG_1(IMO_TR)

#define INITIALIZE_3_3V	INITIALIZE_3(0xf8,0xf9)
#define INITIALIZE_3_5V	INITIALIZE_3(0xfc,0xfd)

#define SET_BANK_NUM(n) \
  WRITE_REG(CPU_F,CPU_F_XIO) \
  WRITE_REG((FLS_PR1) & 0xff,n) \
  WRITE_REG(CPU_F,0)

#define SET_BLOCK_NUM(n) \
  WRITE_MEM(BLOCKID,n)

/*
 * Note sure why we have this exception when setting POINTER. In factm we
 * shouldn't even need to set it.
 */

#define CHECKSUM_SETUP(blocks) \
  VECTOR_PTROP_PREAMBLE(blocks == 128 ? 0 : 0x80) \
  WRITE_MEM(BLOCKID,(blocks) & 0xff) \
  VECTOR_SSC(SSC_Checksum)

#define VERIFY_SETUP \
  VECTOR_PTROP_PREAMBLE(0x80) \
  VECTOR_SSC(SSC_ReadBlock)

#define PROGRAM_BLOCK(clock,delay) \
  WRITE_MEM(CLOCK,clock) \
  WRITE_MEM(DELAY,delay) \
  VECTOR_PTROP_PREAMBLE(0x80) \
  VECTOR_SSC(SSC_WriteBlock)

#define PROGRAM_BLOCK_REGULAR \
  PROGRAM_BLOCK(0x54,0x56)
#define PROGRAM_BLOCK_CY8C27xxx \
  PROGRAM_BLOCK(0x15,0x56)

#define READ_BYTE(addr) \
  READ_MEM((addr) | 0x80)

#define ERASE \
  WRITE_MEM(CLOCK,0x15) \
  WRITE_MEM(DELAY,0x56) \
  VECTOR_PTROP_PREAMBLE(0x80) \
  VECTOR_SSC(SSC_EraseAll)

#define ID_SETUP \
  WRITE_REG(CPU_F,CPU_F_XIO) \
  WRITE_REG(OSC_CR0 & 0xff,2) \
  VECTOR_PTROP_PREAMBLE(0x80) \
  WRITE_MEM(BLOCKID,0) \
  VECTOR_SSC(SSC_TableRead)

#define SECURE(clock,delay) \
  WRITE_MEM(CLOCK,clock) \
  WRITE_MEM(DELAY,delay) \
  VECTOR_PTROP_PREAMBLE(0x80) \
  VECTOR_SSC(SSC_ProtectBlock)

#define SECURE_REGULAR \
  SECURE(0x54,0x56)
#define SECURE_CY8C27xxx \
  SECURE(0x15,0x56)

#define WRITE_BYTE(addr,data) \
  WRITE_MEM((addr) | 0x80,data)

#define READ_CHECKSUM \
  READ_MEM(0xf9) \
  READ_MEM(0xf8) \

#define READ_ID_WORD \
  READ_MEM(0xf8) \
  READ_MEM(0xf9) \

/* Undocumented */

#define VERIFY_SECURE_SETUP \
  VECTOR_PTROP_PREAMBLE(0x80) \
  WRITE_MEM(RESERVED_FD,0) \
  WRITE_MEM(RESERVED_FF,0) \
  VECTOR_SSC(SSC_ReadProtection)

#endif /* !VECTORS_H */
