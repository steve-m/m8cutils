/*
 * macro.h - Macro handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

/*
 * Note: there is no macro.c file. All the functions are defined in m8c.l
 */

#ifndef MACRO_H
#define MACRO_H


struct macro {
    const char *name;
    int num_args;
    char *body;
};

struct id;

void begin_macro(const struct id *id);
void end_macro(void);
void expand_macro(const struct id *id);

#endif /* !MACRO_H */
