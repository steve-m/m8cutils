/*
 * chips.h - CY8C2xxxx chip database
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef CHIPS_H
#define CHIPS_H


#include <stdint.h>
#include <stdio.h>


struct chip {
    const char *name;
    uint16_t id;
    int banks;
    int blocks;
    int cy8c27xxx;
};


void chip_list(FILE *file);
const struct chip *chip_by_name(const char *name);
const struct chip *chip_by_id(const uint16_t id);

#endif /* !CHIPS_H */
