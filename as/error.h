/*
 * error.h - Error reporting
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>


struct loc {
    const char *file;
    int line;
};

extern struct loc current_loc;


void set_file(char *name);

void __attribute__((noreturn)) vlerrorf(const struct loc *loc,const char *fmt,
  va_list ap);
void __attribute__((noreturn)) lerrorf(const struct loc *loc,const char *fmt,
  ...);
void __attribute__((noreturn)) yyerror(const char *s);
void __attribute__((noreturn)) yyerrorf(const char *fmt,...);
void __attribute__((noreturn)) extension(const char *name);
void __attribute__((noreturn)) extensions(const char *name);

void error_init(void);
void error_cleanup(void);

#endif /* !ERROR_H */
