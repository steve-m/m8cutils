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


extern FILE *yyin;
extern FILE *next_file;
extern int interactive;
extern int include_default;
extern int saved_stdin;

int yyparse(void);
void yyrestart(FILE *file);
void my_yyrestart(void);

void exit_if_script(int status);


/* ----- Regular stuff ----------------------------------------------------- */


extern volatile int interrupted; /* SIGINT */
extern volatile int running; /* set if we want to be interruptible */
extern const struct chip *chip;
extern int ice;
extern jmp_buf error_env;


void exception(const char *msg,...)
  __attribute__((format(printf,1,2)));

void __attribute__((noreturn)) yyerrorf(const char *fmt,...)
  __attribute__((format(printf,1,2)));
void __attribute__((noreturn)) yyerror(const char *s);

#endif /* !SIM_H */
