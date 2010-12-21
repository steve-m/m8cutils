/*
 * error-as.h - Error reporting for m8cas
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef ERROR_AS_H
#define ERROR_AS_H

#include "error.h"


extern int allow_extensions;


void __attribute__((noreturn)) no_extension(const char *name);
void __attribute__((noreturn)) no_extensions(const char *name);
#define extension(name) (allow_extensions ? 0 : (no_extension(name), 0))
#define extensions(name) (allow_extensions ? 0 : (no_extensions(name), 0))

#endif /* !ERROR_AS_H */
