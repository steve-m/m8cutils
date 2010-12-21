/*
 * error-as.c - Error reporting for m8cas
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include "error-as.h"


int allow_extensions = 0;


void __attribute__((noreturn)) no_extension(const char *name)
{
    lerrorf(&current_loc,"%s is a non-standard extension",name);
}


void __attribute__((noreturn)) no_extensions(const char *name)
{
    lerrorf(&current_loc,"%s are a non-standard extension",name);
}
