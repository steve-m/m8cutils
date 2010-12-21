/*
 * cy8c2regs.h - Register definitions of the CY8C2 microcontroller family
 * 
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

#ifndef CY8C2REGS_H
#define CY8C2REGS_H

#define REG_CPU_A	0xf0	/* undocumented */
#define REG_CPU_PCL	0xf4	/* undocumented */
#define REG_CPU_PCH	0xf5	/* undocumented */
#define REG_CPU_SP	0xf6	/* undocumented */
#define REG_CPU_F	0xf7
#define REG_CPU_CODE0	0xf8	/* undocumented */
#define REG_CPU_CODE1	0xf9	/* undocumented */
#define REG_CPU_CODE2	0xfa	/* undocumented */
#define REG_CPU_SCR0	0xff

#define REG_OSC_CR0	0x1e0
#define REG_IMO_TR	0x1ea
#define REG_ILO_TR	0x1e9

#define CPU_F_XIO	0x10

#define CPU_SCR0_GIES	0x80
#define CPU_SCR0_WDRS	0x20
#define CPU_SCR0_PORS	0x10
#define CPU_SCR0_Sleep	0x08
#define CPU_SCR0_STOP	0x01

#endif /* !CY8C2REGS_H */
