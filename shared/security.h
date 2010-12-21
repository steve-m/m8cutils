/*
 * security.h - Check protection bits
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef SECURITY_H
#define SECURITY_H


#include <stdio.h>

#include "chips.h"


void set_security(int block,int mode) ;

void read_security(const char *name);

/*
 * "check_security" looks for inconsistencies in the protection bits and warns
 * about them. It also applies padding where needed. If "chips" is NULL, the
 * number of protection bits is not checked against flash size.
 */

void check_security(const struct chip *chip);

#endif /* SECURITY_H */
