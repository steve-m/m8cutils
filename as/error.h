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
extern int allow_extensions;


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
void __attribute__((noreturn)) no_extension(const char *name);
void __attribute__((noreturn)) no_extensions(const char *name);
#define extension(name) (allow_extensions ? 0 : (no_extension(name), 0))
#define extensions(name) (allow_extensions ? 0 : (no_extensions(name), 0))

void error_init(void);
void error_cleanup(void);

#endif /* !ERROR_H */
