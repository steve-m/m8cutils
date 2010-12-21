/*
 * error.h - Error reporting for programming languages
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


static inline const char *get_file(const struct loc *loc)
{
    return loc->file;
}


static inline int get_line(const struct loc *loc)
{
    return loc->line;
}


void set_file(char *name);

void __attribute__((noreturn)) vlerrorf(const struct loc *loc,const char *fmt,
  va_list ap);
void __attribute__((noreturn)) lerrorf(const struct loc *loc,const char *fmt,
  ...)
  __attribute__((format(printf,2,3)));
void __attribute__((noreturn)) yyerror(const char *s);
void __attribute__((noreturn)) yyerrorf(const char *fmt,...)
  __attribute__((format(printf,1,2)));

void vlwarnf(const struct loc *loc,const char *fmt,va_list ap);
void lwarnf(const struct loc *loc,const char *fmt,...)
  __attribute__((format(printf,2,3)));
void yywarn(const char *s);
void yywarnf(const char *fmt,...)
  __attribute__((format(printf,1,2)));

void error_init(void);
void error_cleanup(void);

#endif /* !ERROR_H */
