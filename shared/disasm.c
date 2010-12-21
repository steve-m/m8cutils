/*
 * disasm.c - M8C disassembler
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "util.h"
#include "symmap.h"
#include "disasm.h"


const char m8c_cycles[256] = {
     15,  4,  6,  7,  7,  8,  9, 10,  4,  4,  6,  7,  7,  8,  9, 10,	/* 0 */
      4,  4,  6,  7,  7,  8,  9, 19,  5,  4,  6,  7,  7,  8,  9, 10,	/* 1 */
      5,  4,  6,  7,  7,  8,  9, 10, 11,  4,  6,  7,  7,  8,  9, 10,	/* 2 */
      9,  4,  6,  7,  7,  8,  9, 10,  5,  5,  7,  8,  8,  9, 10, 10,	/* 3 */
      4,  9, 10,  9, 10,  9, 10,  8,  9,  9, 10,  5,  7,  7,  5,  4,	/* 4 */
      4,  5,  6,  5,  6,  8,  9,  4,  6,  7,  5,  4,  4,  6,  7, 10,	/* 5 */
      5,  6,  8,  9,  4,  7,  8,  4,  7,  8,  4,  7,  8,  4,  7,  8,	/* 6 */
      4,  4,  4,  4,  4,  4,  7,  8,  4,  4,  7,  8, 13,  7, 10,  8,	/* 7 */
      5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,	/* 8 */
     11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,	/* 9 */
      5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,	/* A */
      5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,	/* B */
      5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,	/* C */
      5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,	/* D */
      7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,	/* E */
     13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13	/* F */
};

const char m8c_bytes[256] = {
     1, 2, 2, 2, 2, 2, 3, 3, 1, 2, 2, 2, 2, 2, 3, 3,	/* 0 */
     1, 2, 2, 2, 2, 2, 3, 3, 1, 2, 2, 2, 2, 2, 3, 3,	/* 1 */
     1, 2, 2, 2, 2, 2, 3, 3, 1, 2, 2, 2, 2, 2, 3, 3,	/* 2 */
     1, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 2, 2,	/* 3 */
     1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 2, 2, 1, 1,	/* 4 */
     2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 1, 1, 2, 2, 3, 	/* 5 */
     2, 2, 3, 3, 1, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2,	/* 6 */
     2, 2, 2, 1, 1, 1, 2, 2, 1, 1, 2, 2, 3, 3, 1, 1,	/* 7 */
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 8 */
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 9 */
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* A */
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* B */
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* C */
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* D */
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* E */
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2	/* F */
};


#define P(n,...) \
  finish(snprintf(buf,buf_size,__VA_ARGS__),buf_size); return n;

#define ONE(m)		P(1,m);

#define SI_A(m)		P(2,#m "\tA,0x%02X",c[1]);
#define SI_F(m)		P(2,#m "\tF,0x%02X",c[1]);
#define SI_X(m)		P(2,#m "\tX,0x%02X",c[1]);
#define SI_SP(m)	P(2,#m "\tSP,0x%02X",c[1]);

#define U_A(m)		P(1,#m "\tA");
#define U_X(m)		P(1,#m "\tX");
#define U_SD_M(m)	P(2,#m "\t[%s]",ram(c[1]));
#define U_SI_M(m)	P(2,#m "\t[X+0x%02X]%s",c[1],opt_ram(c[1]));

#define SD_A_M(m)	P(2,#m "\tA,[%s]",ram(c[1]));
#define SI_A_M(m)	P(2,#m "\tA,[X+0x%02X]%s",c[1],opt_ram(c[1]));
#define SD_A_R(m)	P(2,#m "\tA,REG[0x%02X]",c[1]);
#define SI_A_R(m)	P(2,#m "\tA,REG[X+0x%02X]",c[1]);
#define DD_A_M(m)	P(2,#m "\t[%s],A",ram(c[1]));
#define DI_A_M(m)	P(2,#m "\t[X+0x%02X],A%s",c[1],opt_ram(c[1]));
#define DD_A_R(m)	P(2,#m "\tREG[0x%02X],A",c[1]);
#define DI_A_R(m)	P(2,#m "\tREG[X+0x%02X],A",c[1]);
#define SD_X_M(m)	P(2,#m "\tX,[%s]",ram(c[1]));
#define SI_X_M(m)	P(2,#m "\tX,[X+0x%02X]%s",c[1],opt_ram(c[1]));
#define DD_X_M(m)	P(2,#m "\t[%s],X",ram(c[1]));
#define DI_X_M(m)	P(2,#m "\t[X+0x%02X],X%s",c[1],opt_ram(c[1]));
#define DD_SI_M(m)	P(3,#m "\t[%s],0x%02X",ram(c[1]),c[2]);
#define DI_SI_M(m)	P(3,#m "\t[X+0x%02X],0x%02X%s",c[1],c[2],opt_ram(c[1]));
#define DD_SI_R(m)	P(3,#m "\tREG[0x%02X],0x%02X",c[1],c[2]);
#define DI_SI_R(m)	P(3,#m "\tREG[X+0x%02X],0x%02X",c[1],c[2]);
#define DD_SD_M(m)	P(3,#m "\t[%s],[%s]",ram(c[1]),ram(c[2]));


#define PC12(off,ref) \
  ((pc+(ref)+((off) & 0x800 ? 0xf000 : 0)+(off)) & 0xffff)


#define J12(m,r) P(2,#m "\t%s",label(pc,PC12(((c[0] & 0xf) << 8) | c[1],r)));
#define J16(m)	P(3,#m "\t%s",label(pc,c[1] << 8 | c[2]));


static const char *label(uint16_t pc,uint16_t addr)
{
    static char buf[7];
    char *tmp = NULL;
    int tmp_size = 0,len;
    const char *sym;

    sym = sym_by_value(addr,SYM_ATTR_ROM,NULL);
    if (!sym) {
	sprintf(buf,"0x%04X",addr);
	return buf;
    }
    if (!sym_is_redefinable(sym))
	return sym;
    len = strlen(sym);
    if (len+1 >= tmp_size) {
	tmp = realloc_type(tmp,len+2);
	tmp_size = len+2;
    }
    sprintf(tmp,"%s%c",sym,addr <= pc ? 'b' : 'f');
    return tmp;
}


/*
 * @@@ We should also look for values in other banks. Furthermore, the user of
 * the disassembler should be able to indicate the current banking setup, e.g.,
 * in the case of simulation.
 */

static const char *ram(uint8_t addr)
{
    static char buf[2][5];
    static int buf_sel = 0;
    const char *sym;

    sym = sym_by_value(addr,SYM_ATTR_RAM,NULL);
    if (sym && !sym_is_redefinable(sym))
	return sym;
    /*
     * We alternate between two buffers, so that we can handle up to two
     * pending invocations of "ram", as in "mov [foo],[bar]".
     */
    buf_sel = !buf_sel;
    sprintf(buf[buf_sel],"0x%02X",addr);
    return buf[buf_sel];
}


static const char *opt_ram(uint8_t addr)
{
    static char *buf = NULL;
    const char *sym;

    if (buf) {
	free(buf);
	buf = NULL;
    }
    sym = sym_by_value(addr,SYM_ATTR_RAM,NULL);
    if (!sym)
	return "";
    buf = malloc(strlen(sym)+4);
    if (!buf) {
	perror("malloc");
	exit(1);
    }
    sprintf(buf,"\t; %s",sym);
    return buf;
}


static void finish(ssize_t wrote,size_t buf_size)
{
    if (wrote < 0) {
	perror("snprintf");
	exit(1);
    }
    if (wrote >= buf_size) {
	fprintf(stderr,"buffer overflow (caught): %d >= %d\n",
	  (int) wrote,(int) buf_size);
	exit(1);
    }
}


int m8c_disassemble(uint16_t pc,const uint8_t *c,char *buf,size_t buf_size)
{
    
    switch (c[0]) {
	case 0x00:	ONE("SSC");
	case 0x01:	SI_A(ADD);
	case 0x02:	SD_A_M(ADD);
	case 0x03:	SI_A_M(ADD);
	case 0x04:	DD_A_M(ADD);
	case 0x05:	DI_A_M(ADD);
	case 0x06:	DD_SI_M(ADD);
	case 0x07:	DI_SI_M(ADD);
	case 0x08:	U_A(PUSH);
	case 0x09:	SI_A(ADC);
	case 0x0a:	SD_A_M(ADC);
	case 0x0b:	SI_A_M(ADC);
	case 0x0c:	DD_A_M(ADC);
	case 0x0d:	DI_A_M(ADC);
	case 0x0e:	DD_SI_M(ADC);
	case 0x0f:	DI_SI_M(ADC);

	case 0x10:	U_X(PUSH);
	case 0x11:	SI_A(SUB);
	case 0x12:	SD_A_M(SUB);
	case 0x13:	SI_A_M(SUB);
	case 0x14:	DD_A_M(SUB);
	case 0x15:	DI_A_M(SUB);
	case 0x16:	DD_SI_M(SUB);
	case 0x17:	DI_SI_M(SUB);
	case 0x18:	U_A(POP);
	case 0x19:	SI_A(SBB);
	case 0x1a:	SD_A_M(SBB);
	case 0x1b:	SI_A_M(SBB);
	case 0x1c:	DD_A_M(SBB);
	case 0x1d:	DI_A_M(SBB);
	case 0x1e:	DD_SI_M(SBB);
	case 0x1f:	DI_SI_M(SBB);

	case 0x20:	U_X(POP);
	case 0x21:	SI_A(AND);
	case 0x22:	SD_A_M(AND);
	case 0x23:	SI_A_M(AND);
	case 0x24:	DD_A_M(AND);
	case 0x25:	DI_A_M(AND);
	case 0x26:	DD_SI_M(AND);
	case 0x27:	DI_SI_M(AND);
	case 0x28:	ONE("ROMX");
	case 0x29:	SI_A(OR);
	case 0x2a:	SD_A_M(OR);
	case 0x2b:	SI_A_M(OR);
	case 0x2c:	DD_A_M(OR);
	case 0x2d:	DI_A_M(OR);
	case 0x2e:	DD_SI_M(OR);
	case 0x2f:	DI_SI_M(OR);

	case 0x30:	ONE("HALT");
	case 0x31:	SI_A(XOR);
	case 0x32:	SD_A_M(XOR);
	case 0x33:	SI_A_M(XOR);
	case 0x34:	DD_A_M(XOR);
	case 0x35:	DI_A_M(XOR);
	case 0x36:	DD_SI_M(XOR);
	case 0x37:	DI_SI_M(XOR);
	case 0x38:	SI_SP(ADD);
	case 0x39:	SI_A(CMP);
	case 0x3a:	SD_A_M(CMP);
	case 0x3b:	SI_A_M(CMP);
	case 0x3c:	DD_SI_M(CMP);
	case 0x3d:	DI_SI_M(CMP);
	case 0x3e:	P(2,"MVI\tA,[[0x%02X]++]",c[1]);
	case 0x3f:	P(2,"MVI\t[[0x%02X]++],A",c[1]);

	case 0x40:	ONE("NOP");
	case 0x41:	DD_SI_R(AND);
	case 0x42:	DI_SI_R(AND);
	case 0x43:	DD_SI_R(OR);
	case 0x44:	DI_SI_R(OR);
	case 0x45:	DD_SI_R(XOR);
	case 0x46:	DI_SI_R(XOR);
	case 0x47:	DD_SI_M(TST);
	case 0x48:	DI_SI_M(TST);
	case 0x49:	DD_SI_R(TST);
	case 0x4a:	DI_SI_R(TST);
	case 0x4b:	ONE("SWAP\tA,X");
	case 0x4c:	SD_A_M(SWAP);
	case 0x4d:	SD_X_M(SWAP);
	case 0x4e:	ONE("SWAP\tA,SP");
	case 0x4f:	ONE("MOV\tX,SP");

	case 0x50:	SI_A(MOV);
	case 0x51:	SD_A_M(MOV);
	case 0x52:	SI_A_M(MOV);
	case 0x53:	DD_A_M(MOV);
	case 0x54:	DI_A_M(MOV);
	case 0x55:	DD_SI_M(MOV);
	case 0x56:	DI_SI_M(MOV);
	case 0x57:	SI_X(MOV);
	case 0x58:	SD_X_M(MOV);
	case 0x59:	SI_X_M(MOV);
	case 0x5a:	DD_X_M(MOV);
	case 0x5b:	ONE("MOV\tA,X");
	case 0x5c:	ONE("MOV\tX,A");
	case 0x5d:	SD_A_R(MOV);
	case 0x5e:	SI_A_R(MOV);
	case 0x5f:	DD_SD_M(MOV);

	case 0x60:	DD_A_R(MOV);
	case 0x61:	DI_A_R(MOV);
	case 0x62:	DD_SI_R(MOV);
	case 0x63:	DI_SI_R(MOV);
	case 0x64:	U_A(ASL);
	case 0x65:	U_SD_M(ASL);
	case 0x66:	U_SI_M(ASL);
	case 0x67:	U_A(ASR);
	case 0x68:	U_SD_M(ASR);
	case 0x69:	U_SI_M(ASR);
	case 0x6a:	U_A(RLC);
	case 0x6b:	U_SD_M(RLC);
	case 0x6c:	U_SI_M(RLC);
	case 0x6d:	U_A(RRC);
	case 0x6e:	U_SD_M(RRC);
	case 0x6f:	U_SI_M(RRC);

	case 0x70:	SI_F(AND);
	case 0x71:	SI_F(OR);
	case 0x72:	SI_F(XOR);
	case 0x73:	U_A(CPL);
	case 0x74:	U_A(INC);
	case 0x75:	U_X(INC);
	case 0x76:	U_SD_M(INC);
	case 0x77:	U_SI_M(INC);
	case 0x78:	U_A(DEC);
	case 0x79:	U_X(DEC);
	case 0x7a:	U_SD_M(DEC);
	case 0x7b:	U_SI_M(DEC);
	case 0x7c:	J16(LCALL);
	case 0x7d:	J16(LJMP);
	case 0x7e:	ONE("RETI");
	case 0x7f:	ONE("RET");

	case 0x80 ... 0x8f: J12(JMP,1);
	case 0x90 ... 0x9f: J12(CALL,2);
	case 0xa0 ... 0xaf: J12(JZ,1);
	case 0xb0 ... 0xbf: J12(JNZ,1);
	case 0xc0 ... 0xcf: J12(JC,1);
	case 0xd0 ... 0xdf: J12(JNC,1);
	case 0xe0 ... 0xef: J12(JACC,1);
	case 0xf0 ... 0xff: J12(INDEX,2);

	default:
	    abort(); /* should be impossible */
    }
}
