/*
 * ice.h - Use of programming mode as an ICE
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef ICE_H
#define ICE_H

#include <stdint.h>

#include "chips.h"


const struct chip *ice_open(const char *dev,const char *programmer,
  const char *chip_name,int voltage);
  /* warning: "dev" and "chip" can be NULL */
void ice_write(int reg,uint8_t value);
uint8_t ice_read(int reg);
void ice_close(void);

#endif /* !ICE_H */
