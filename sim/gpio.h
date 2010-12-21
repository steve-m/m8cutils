/*
 * gpio.c - General-purpose I/O
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdio.h>


void gpio_ice_connect(int port,uint8_t set);
void gpio_ice_disconnect(int port,uint8_t set);
void gpio_drive(int port,uint8_t mask,uint8_t value);
void gpio_drive_z(int port,uint8_t mask);
void gpio_drive_r(int port,uint8_t mask,uint8_t value);
void gpio_set(int port,uint8_t mask,uint8_t value);
void gpio_set_r(int port,uint8_t mask,uint8_t value);
void gpio_set_z(int port,uint8_t mask);
void gpio_set_analog(int port,uint8_t mask);
void gpio_show(FILE *file,int port,uint8_t mask);
void gpio_init(void);

#endif /* !GPIO_H */
