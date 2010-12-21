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
 * With "defined" and "used", we have the following combinations:
 *
 * defined, !used:	use follows definition, we're before the use
 * !defined, used:	definition follows use, we're before the definition
 * defined, used:	we're path definition and first use
 * !defined, !used:	this isn't a label, but a keyword or section name from
 *			an AREA directive
 */

struct area;

struct id {
    char *name;		/* jrb_insert_str doesn't want this to be "const" :-(*/
    struct op *value;
    int defined;	/* "value" contains the label's value */
    int global;		/* label is global */
    int used;		/* the label has actually been referenced */
    const struct area *area; /* area in which the label is defined (NULL if
			   this is an assignment) */
    struct loc loc;	/* location of definition, or first reference if
			   undefined */
};


struct id *make_id(char *name);
void assign(struct id *id,struct op *value);
void id_init(void);
void write_ids(FILE *file);
void id_cleanup(void);

#endif /* !ID_H */
