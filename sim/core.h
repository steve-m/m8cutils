/*              
 * core.h - M8C core simulation
 *                
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */               


#ifndef CORE_H
#define CORE_H

#include <stdint.h>


extern uint8_t a;

union f_union {
    uint8_t f;
    struct {
	unsigned gie:1;
	unsigned z:1;
	unsigned c:1;
	unsigned pad0:1;
	unsigned xio:1;
	unsigned pad1:1;
	unsigned pgmode:2;
    } f_bits;
};

extern union f_union f_union;

#define zf      f_union.f_bits.z
#define cf      f_union.f_bits.c
#define xio     f_union.f_bits.xio
#define gie     f_union.f_bits.gie
#define pgmode  f_union.f_bits.pgmode

#define f       f_union.f


extern uint8_t ram[2048];
extern uint8_t *stack;
extern uint8_t rom[0x10000];

extern uint16_t pc;
extern uint8_t x;
extern uint8_t sp;


uint32_t m8c_run(uint32_t cycles);
void m8c_init(void);

#endif /* !CORE_H */
