/*
 * file.h - File I/O
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <sys/types.h>

#include "cy8c2prog.h"


extern size_t file_size;
extern int have_security; /* non-zero if "security" contains valid data */

extern uint8_t program[PROGRAM_SIZE];
extern uint8_t security[SECURITY_SIZE];

uint16_t do_checksum(void);
void read_file(const char *name,int binary);
void write_file(const char *name,int binary);

#endif /* !FILE_H */
