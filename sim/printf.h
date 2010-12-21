/*
 * printf.h - Simple printf function
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef PRINTF_H
#define PRINTF_H

#include <stdint.h>
#include <stdio.h>


struct nlist {
    uint32_t n;
    struct nlist *next;
};

void do_printf(FILE *file,const char *fmt,const struct nlist *args);

#endif /* !PRINTF_H */
