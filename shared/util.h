/*
 * util.h - Common utility functions
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string.h>


#define alloc_type(t) \
  ({ t *alloc_type_tmp = (t *) malloc(sizeof(t)); \
     if (!alloc_type_tmp) abort(); \
     alloc_type_tmp; })

#define stralloc(s) \
  ({ char *stralloc_tmp = strdup(s); \
     if (!stralloc_tmp) abort(); \
     stralloc_tmp; })

#define ctz(v) (ffs(v)-1)

#endif /* !UTIL_H */
