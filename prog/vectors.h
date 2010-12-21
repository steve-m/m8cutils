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


#include "cy8c2regs.h"

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

#define KEY1		0xf8
#define KEY2		0xf9
#define	BLOCKID		0xfa
#define POINTER		0xfb
#define CLOCK		0xfc
#define DELAY		0xfe

#define MAGIC_EXEC	0x12	/* magic number to trigger execution */

#define EXEC_VECTOR_VALUE \
  VECTOR_VALUE(PROG_OP_WRITE_REG,REG_CPU_SCR0,MAGIC_EXEC)
#define EXEC_VECTOR_SSC_VALUE  \
  VECTOR_VALUE(PROG_OP_WRITE_REG | (PROG_SSC_FLAG >> 16),REG_CPU_SCR0,MAGIC_EXEC)
#define EXEC_VECTOR	OUTPUT_VECTOR(EXEC_VECTOR_VALUE)
#define EXEC_VECTOR_SSC	OUTPUT_VECTOR(EXEC_VECTOR_SSC_VALUE)

#define SSC_SWBootReset		0
#define SSC_ReadBlock		1
#define SSC_WriteBlock		2
#define SSC_EraseBlock		3
#define SSC_ProtectBlock	4
#define SSC_EraseAll		5
#define SSC_TableRead		6
#define SSC_Checksum		7
#define SSC_Calibrate0		8
#define SSC_Calibrate1		9

#define OP_SSC			0x00
#define OP_HALT			0x30
#define OP_NOP			0x40
#define OP_MOV_A_SD_MEM		0x51	/* MOV A,[expr] */
#define OP_MOV_A_DD_REG		0x60	/* MOV reg[expr],A */

#define VECTOR_PTROP_PREAMBLE \
  WRITE_REG(REG_CPU_F,0) \
  WRITE_REG(REG_CPU_SP,0) \
  WRITE_MEM(KEY1,0x3a) \
  WRITE_MEM(KEY2,3) \
  WRITE_REG(REG_CPU_PCH,0) \
  WRITE_REG(REG_CPU_PCL,3) \
  WRITE_MEM(POINTER,0x80) \
  WRITE_REG(REG_CPU_CODE1,OP_HALT) \
  WRITE_REG(REG_CPU_CODE2,OP_NOP) \

#define VECTOR_SSC(code) \
  WRITE_REG(REG_CPU_A,code) \
  WRITE_REG(REG_CPU_CODE0,OP_SSC) \
  EXEC_VECTOR_SSC

#define INITIALIZE_1 \
  INIT_VECTOR \
  ZERO_VECTOR \
  ZERO_VECTOR \
  ZERO_VECTOR \
  ZERO_VECTOR \
  ZERO_VECTOR \
  VECTOR_PTROP_PREAMBLE \
  VECTOR_SSC(SSC_Calibrate1)

#define INITIALIZE_2 \
  VECTOR_PTROP_PREAMBLE \
  WRITE_MEM(BLOCKID,1) \
  VECTOR_SSC(SSC_TableRead)

#define VECTOR_MOV_A_SD_MEM(addr) \
  WRITE_REG(REG_CPU_F,0) \
  WRITE_REG(REG_CPU_PCH,0) \
  WRITE_REG(REG_CPU_PCL,3) \
  WRITE_REG(REG_CPU_SP,8) \
  WRITE_REG(REG_CPU_CODE0,OP_MOV_A_SD_MEM) \
  WRITE_REG(REG_CPU_CODE1,addr) \
  WRITE_REG(REG_CPU_CODE2,OP_HALT) \
  EXEC_VECTOR \
  ZERO_VECTOR

#define VECTOR_MOV_A_DD_REG_1(addr) \
  WRITE_REG(REG_CPU_F,0) \
  WRITE_REG(REG_CPU_PCH,0) \
  WRITE_REG(REG_CPU_PCL,3) \
  WRITE_REG(REG_CPU_SP,8) \
  WRITE_REG(REG_CPU_CODE0,OP_MOV_A_DD_REG) \
  WRITE_REG(REG_CPU_CODE1,(addr) & 0xff) \
  WRITE_REG(REG_CPU_CODE2,OP_HALT) \
  WRITE_REG(REG_CPU_F,CPU_F_XIO) \
  EXEC_VECTOR \
  ZERO_VECTOR

#define INITIALIZE_3(imo,ilo) \
  VECTOR_MOV_A_SD_MEM(imo) \
  VECTOR_MOV_A_DD_REG_1(REG_IMO_TR) \
  VECTOR_MOV_A_SD_MEM(ilo) \
  VECTOR_MOV_A_DD_REG_1(REG_ILO_TR)

#define INITIALIZE_3_3V	INITIALIZE_3(0xf8,0xf9)
#define INITIALIZE_3_5V	INITIALIZE_3(0xfc,0xfd)

#define SET_BLOCK_NUM(n) \
  WRITE_MEM(BLOCKID,n)

#define CHECKSUM_SETUP(blocks) \
  VECTOR_PTROP_PREAMBLE \
  WRITE_MEM(BLOCKID,blocks & 0xff) \
  VECTOR_SSC(SSC_Checksum)

#define VERIFY_SETUP \
  VECTOR_PTROP_PREAMBLE \
  VECTOR_SSC(SSC_ReadBlock)

#define PROGRAM_BLOCK(clock,delay) \
  WRITE_MEM(CLOCK,clock) \
  WRITE_MEM(DELAY,delay) \
  VECTOR_PTROP_PREAMBLE \
  VECTOR_SSC(SSC_WriteBlock)

#define PROGRAM_BLOCK_REGULAR \
  PROGRAM_BLOCK(0x54,0x56)
#define PROGRAM_BLOCK_CY8C27xxx \
  PROGRAM_BLOCK(0x15,0x56)

#define READ_BYTE(addr) \
  READ_MEM(addr | 0x80)

#define ERASE \
  WRITE_MEM(CLOCK,0x15) \
  WRITE_MEM(DELAY,0x56) \
  VECTOR_PTROP_PREAMBLE \
  VECTOR_SSC(SSC_EraseAll)

#define ID_SETUP \
  WRITE_REG(REG_CPU_F,CPU_F_XIO) \
  WRITE_REG(REG_OSC_CR0 & 0xff,2) \
  VECTOR_PTROP_PREAMBLE \
  WRITE_MEM(BLOCKID,0) \
  VECTOR_SSC(SSC_TableRead)

#define SECURE(clock,delay) \
  WRITE_MEM(CLOCK,clock) \
  WRITE_MEM(DELAY,delay) \
  VECTOR_PTROP_PREAMBLE \
  VECTOR_SSC(SSC_ProtectBlock)

#define SECURE_REGULAR \
  SECURE(0x54,0x56)
#define SECURE_CY8C27xxx \
  SECURE(0x15,0x56)

#define WRITE_BYTE(addr,data) \
  WRITE_MEM(addr | 0x80,data)

#define READ_CHECKSUM \
  READ_MEM(0xf9) \
  READ_MEM(0xf8) \

#define READ_ID_WORD \
  READ_MEM(0xf8) \
  READ_MEM(0xf9) \

#define ID_CY8C27143	0x0009
#define	ID_CY8C27243	0x000a
#define	ID_CY8C27443	0x000b
#define ID_CY8C27543	0x000c
#define ID_CY8C27643	0x000d
#define ID_CY8C24123	0x0012
#define	ID_CY8C24223	0x0013
#define ID_CY8C24423	0x0014
#define ID_CY8C22113	0x000f
#define ID_CY8C22213	0x0010
#define ID_CY8C21123	0x0017
#define ID_CY8C21223	0x0018
#define	ID_CY8C21323	0x0019
#define ID_CY8C21234	0x0036
#define	ID_CY8C21334	0x0037
#define	ID_CY8C21434	0x0038
#define	ID_CY8C21534	0x0040
#define ID_CY8C21634	0x0049

#endif /* !VECTORS_H */
