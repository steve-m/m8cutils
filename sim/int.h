/*
 * int.h - Interrupt handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef INT_H
#define INT_H

#include <stdint.h>


extern uint8_t int_vc;


void int_handle(void);
void int_poll(void);
void int_set(int number);
void int_clear(int number);
void int_set_ice(int number,int connected);
void int_init(void);

#endif /* !INT_H */
