/*
 * input.h - Macro and include file handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

/*
 * Note: there is no macro.c file. All the functions are defined in m8c.l
 */

#ifndef INPUT_H
#define INPUT_H


#include <string.h>

#include "util.h"
#include "error.h"


struct dyn_string {
    char *s;
    int size;
};

struct macro {
    const char *name;
    struct loc loc;
    int num_args;
    struct dyn_string body;
    struct macro *next;
};


struct id;


static inline void init_string(struct dyn_string *ds)
{
    ds->s = alloc_type(char);
    ds->size = 1;
    *ds->s = 0;
}


static inline void add_string_n(struct dyn_string *ds,const char *s,int n)
{
    ds->s = realloc_type(ds->s,ds->size+n);
    memcpy(ds->s+ds->size-1,s,n);
    ds->size += n;
    ds->s[ds->size-1] = 0;
}


static inline void add_string(struct dyn_string *ds,const char *s)
{
    int len = strlen(s);

    ds->s = realloc_type(ds->s,ds->size+len);
    memcpy(ds->s+ds->size-1,s,len+1);
    ds->size += len;
}


static inline const char *get_string(const struct dyn_string *ds)
{
    return ds->s;
}


static inline void free_string(struct dyn_string *ds)
{
    free(ds->s);
    ds->s = NULL;
}


void begin_macro(const struct id *id);
void end_macro(void);
void expand_macro(const struct id *id);
void remove_macros(void);

void include_file(const char *name);

#endif /* !INPUT_H */
