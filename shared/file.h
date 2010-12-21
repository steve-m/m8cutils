/*
 * file.h - File I/O for the CY8C2 Utilities
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <sys/types.h>


#define PROGRAM_SIZE	32768
#define BLOCK_SIZE	64
#define SECURITY_SIZE	(PROGRAM_SIZE/BLOCK_SIZE/4)
#define PAD_BYTE	0


extern size_t program_size;
extern size_t security_size; /* zero if we have no security data */

extern uint8_t program[PROGRAM_SIZE];
extern uint8_t security[SECURITY_SIZE];

uint16_t do_checksum(void);
void read_file(const char *name,int binary);
void write_file(const char *name,int binary,int hex);
void pad_file(void);

#endif /* !FILE_H */
