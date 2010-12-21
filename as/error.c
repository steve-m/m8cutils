/*
 * error.c - Error reporting
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
int allow_extensions = 0;


static JRB file_names;


void set_file(char *name)
{
    JRB entry;

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


void __attribute__((noreturn)) no_extension(const char *name)
{
    lerrorf(&current_loc,"%s is a non-standard extension",name);
}


void __attribute__((noreturn)) no_extensions(const char *name)
{
    lerrorf(&current_loc,"%s are a non-standard extension",name);
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
