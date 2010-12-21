/*
 * port.h - Port setup and access
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PORT_H
#define PORT_H

#include <stdint.h>


#define PRTxDR(x)	(4*(x))
#define PRTxDMy(x,y)	(4*(x)+((y) == 0 ? 0x100 : (y) == 1 ? 0x101 : 3))


int decode_value(int bit);
void set_value(int bit,int value);
uint8_t read_reg(int addr);
void check_target(int current_bit,int current_value);
void commit(int current_bit,int current_value);

void init_ports(void);

#endif /* PORT_H */
