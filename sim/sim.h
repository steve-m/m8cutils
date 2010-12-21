/* 
 * sim.h - M8C simulator
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef SIM_H
#define SIM_H

#include <setjmp.h>
#include <stdio.h>


/* ----- Items for LEX and YACC -------------------------------------------- */


int yyparse(void);
void yyrestart(FILE *file);
void my_yyrestart(void);

extern FILE *yyin;
extern FILE *next_file;


/* ----- Regular stuff ----------------------------------------------------- */


extern volatile int interrupted; /* SIGINT */
extern volatile int running; /* set if we want to be interruptible */
extern const struct chip *chip;
extern int ice;
extern jmp_buf error_env;


void exception(const char *msg,...);

void __attribute__((noreturn)) yyerrorf(const char *fmt,...);
void __attribute__((noreturn)) yyerror(const char *s);

#endif /* !SIM_H */
