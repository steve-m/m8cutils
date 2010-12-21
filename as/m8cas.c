/*
 * m8cas.c - M8C assembler
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdio.h>

#include "error.h"
#include "code.h"


int yyparse(void);


uint8_t program[65536];


int main(void)
{
    int i;

    id_init();
    error_init();
    set_file("<stdin>");
    (void) yyparse();
    resolve();
    id_cleanup();
    error_cleanup();
    for (i = 0; i != highest_pc; i++)
	printf("%02X%c",program[i],i == highest_pc-1 ? '\n' : ' ');
    return 0;
}
