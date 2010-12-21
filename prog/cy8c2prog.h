/*
 * cy8c2prog.h - General programmer for the CY8C2 microcontroller family
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef CY8C2PROG_H
#define C8C2PROG


#define OUTPUT_WIDTH 79

#define PROGRAM_SIZE	16384
#define BLOCK_SIZE	64
#define SECURITY_SIZE	BLOCK_SIZE
#define PAD_BYTE	0


extern int verbose;

#endif /* !CY8C2PROG */
