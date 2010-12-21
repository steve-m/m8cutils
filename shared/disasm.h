/*
 * disasm.h - M8C disassembler
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef DISASM_H
#define DISASM_H

#include <stdint.h>
#include <sys/types.h>


#define MAX_DISASM_LINE	40	/* maximum should be ~20+1 */


extern const char m8c_cycles[256];

int m8c_disassemble(uint16_t pc,const uint8_t *c,char *buf,size_t buf_size);

#endif /* !DISASM_H */
