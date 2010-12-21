/*
 * error.c - Error reporting for programming languages
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "jrb.h"

#include "util.h"
#include "error.h"


struct loc current_loc;


static JRB file_names;


void set_file(char *name)
{
    JRB entry;

    if (!*name)
	name = "<stdin>";
    entry = jrb_find_str(file_names,name);
    if (entry)
	name = jval_v(jrb_val(entry));
    else {
	name = stralloc(name);
	jrb_insert_str(file_names,name,new_jval_v(name));
    }
    current_loc.file = name;
    current_loc.line = 1;
}


/* ----- Errors ------------------------------------------------------------ */


void __attribute__((noreturn)) vlerrorf(const struct loc *loc,const char *fmt,
  va_list ap)
{
    fflush(stdout);
    fprintf(stderr,"%s:%d: ",loc->file,loc->line);
    vfprintf(stderr,fmt,ap);
    putc('\n',stderr);
    exit(1);
}


void __attribute__((noreturn)) lerrorf(const struct loc *loc,const char *fmt,
  ...)
{
    va_list(ap);

    va_start(ap,fmt);
    vlerrorf(loc,fmt,ap);
    va_end(ap);
}


void __attribute__((noreturn)) yyerrorf(const char *fmt,...)
{
    va_list ap;

    va_start(ap,fmt);
    vlerrorf(&current_loc,fmt,ap);
    va_end(ap);
}


void __attribute__((noreturn)) yyerror(const char *s)
{
    lerrorf(&current_loc,"%s",s);
}


/* ----- Warnings ---------------------------------------------------------- */


void  vlwarnf(const struct loc *loc,const char *fmt,va_list ap)
{
    fflush(stdout);
    fprintf(stderr,"%s:%d: warning: ",loc->file,loc->line);
    vfprintf(stderr,fmt,ap);
    putc('\n',stderr);
}


void lwarnf(const struct loc *loc,const char *fmt,...)
{
    va_list(ap);

    va_start(ap,fmt);
    vlwarnf(loc,fmt,ap);
    va_end(ap);
}


void yywarnf(const char *fmt,...)
{
    va_list ap;

    va_start(ap,fmt);
    vlwarnf(&current_loc,fmt,ap);
    va_end(ap);
}


void yywarn(const char *s)
{
    lwarnf(&current_loc,"%s",s);
}


void error_init(void)
{
    file_names = make_jrb();
}


void error_cleanup(void)
{
    JRB entry;

    jrb_traverse(entry,file_names)
	free(jval_v(jrb_val(entry)));
    jrb_free_tree(file_names);
}
