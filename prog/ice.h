/*
 * ice.h - Use of programming mode as an ICE
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef ICE_H
#define ICE_H

#include <stdint.h>


void ice_init(void);
void ice_write(int reg,uint8_t value);
uint8_t ice_read(int reg);

#endif /* !ICE_H */
