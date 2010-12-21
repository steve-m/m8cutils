/*
 * id.h - Identifier handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#ifndef ID_H
#define ID_H

#include <stdio.h>

#include "error.h"


/*
 * With "values" and "used", we have the following combinations:
 *
 * values, !used:	use follows definition, we're before the use
 * !values, used:	definition follows use, we're before the definition
 * values, used:	we're path definition and first use
 * !values, !used:	this isn't a label, but a keyword or section name from
 *			an AREA directive
 */

struct area;

struct value {
    struct op *value;
    const struct area *area; /* area in which the label is defined (NULL if
			   this is an assignment) */
    struct loc loc;	/* location of definition */
    int evaluating;	/* catch recursive definitions */
    struct value *next;
};

struct id {
    char *name;		/* jrb_insert_str doesn't want this to be "const" :-(*/
    struct value *values; /* NULL if none */
    struct value **prev_value; /* pointer to current value, or NULL */
    struct value **pprev_value; /* one further back, for EQU */
    struct value **next_value; /* location where pointer to next value is put */
    int global;		/* label is global */
    int used;		/* label has been referenced (this is only good for
			   detection of undefined labels, not for truly unused
			   ones, since the latter may have multiple instances
			   in the case of redefinable labels) */
    struct id *alias;	/* global label with the same name, NULL if none */
    struct loc loc;	/* location of first reference, used for export */
};


extern int in_equ; /* re-definable labels are treated differently in EQU */


struct id *make_id(char *name);
struct value *declare(struct id *id,const struct area *area,int redefine);
void define(const struct id *id,struct value *val,struct op *value);
void assign(struct id *id,struct op *value,const struct area *area,
  int redefine);
void export(struct id *id);
struct value **id_resolve(struct id *id,int direction);
void id_init(void);
void id_begin_file(void);
void id_end_file(void);
void write_ids(FILE *file);
void id_cleanup(void);

#endif /* !ID_H */
