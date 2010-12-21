/*
 * lang.h - Scripting language of the simulator, common declaration
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef LANG_H
#define LANG_H

#include <stdint.h>


struct lvalue {
    enum lvalue_type {
        lt_ram,
        lt_reg,
        lt_rom,
        lt_a,
        lt_f,
        lt_sp,
        lt_x,
	lt_ice,
    } type;
    uint32_t n;
    uint32_t mask;
};


#endif /* !LANG_H */
