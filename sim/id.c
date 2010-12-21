/*
 * id.c - Identifiers
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>

#include "jrb.h"

#include "util.h"
#include "sim.h"
#include "id.h"


static JRB ids;


struct id *id_lookup(const char *name)
{
    JRB entry;

    entry = jrb_find_str(ids,(char *) name);
    return entry ? jval_v(jrb_val(entry)) : NULL;
}


struct id *id_new(const char *name)
{
    struct id *id;

    if (id_lookup(name))
	abort();
    id = alloc_type(struct id);
    id->name = stralloc(name);
    id->new = 1;
    jrb_insert_str(ids,(char *) id->name,new_jval_v(id));
    return id;
}


void id_reg(struct id *id,uint16_t reg)
{
    id->new = 0;
    id->reg = reg;
    id->field = 0;
}


void id_field(struct id *id,uint16_t reg,uint8_t mask)
{
    id->new = 0;
    id->reg = reg;
    id->field = mask;
}


void id_value(struct id *id,uint8_t value)
{
    id->new = 0;
    id->reg = 0xffff;
    id->field = value;
}


void id_init(void)
{
    ids = make_jrb();
}
