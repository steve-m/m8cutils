/*
 * cpp.h - CPP subprocess
 *
 * Written 2002,2003 by Werner Almesberger, Caltech Netlab FAST project
 */

#ifndef CPP_H
#define CPP_H

void add_cpp_arg(const char *arg);
void add_cpp_Wp(const char *arg);
void run_cpp_on_file(const char *name); /* NULL for stdin */
void run_cpp_on_string(const char *str);
void reap_cpp(void);

#endif /* CPP_H */
