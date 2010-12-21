/*
 * ssc.inc - SSC function codes and memory locations
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef SSC_INC
#define SSC_INC

/*
 * Note: this file is intended for inclusion into C and assembler programs.
 */

/*
 * SSC function codes
 */

#define SSC_SWBootReset		0
#define SSC_ReadBlock		1
#define SSC_WriteBlock		2
#define SSC_EraseBlock		3
#define SSC_ProtectBlock	4	/* partially documented */
#define SSC_EraseAll		5	/* undocumented */
#define SSC_TableRead		6
#define SSC_Checksum		7
#define SSC_Calibrate0		8
#define SSC_Calibrate1		9
#define SSC_ReadProtection	16	/* undocumented */

/*
 * SSC RAM locations
 */

#define KEY1		0xf8
#define KEY2		0xf9
#define	BLOCKID		0xfa
#define POINTER		0xfb
#define CLOCK		0xfc
#define RESERVED_FD	0xfd
#define DELAY		0xfe
#define RESERVED_FF	0xff

#define RETURN_CODE	0xf8

/*
 * Magic value to store in KEY1
 */

#define	KEY1_MAGIC	0x3a

#endif /* !SSC_INC */
