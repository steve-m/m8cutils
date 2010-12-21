/*
 * id.h - Identifier handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef ID_H
#define ID_H

#include "error.h"


struct id {
    char *name;		/* jrb_insert_str doesn't want this to be "const" :-(*/
    struct op *value;
    int defined;
    int global;
    struct loc loc;	/* location of definition, or first reference if
			   undefined */
};


struct id *make_id(char *name);
void assign(struct id *id,struct op *value);
void id_init(void);
void id_cleanup(void);

#endif /* !ID_H */
