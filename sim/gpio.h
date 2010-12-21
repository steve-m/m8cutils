/*
 * gpio.c - General-purpose I/O
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>


void gpio_ice_connect(int port,uint8_t set);
void gpio_ice_disconnect(int port,uint8_t set);
void gpio_drive(int port,int bit,uint8_t mask,uint8_t value);
void gpio_init(void);

#endif /* !GPIO_H */
