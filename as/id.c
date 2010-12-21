/*
 * id.h - Identifier handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jrb.h"

#include "util.h"
#include "error.h"
#include "op.h"
#include "code.h"
#include "id.h"


#define ID_PAD	20	/* in the symbol file, pad all identifiers to 16
			   characters */
#define AREA_PAD 12	/* likewise, for area names */


static struct tree_list {
    JRB tree;
    JRB entry; /* for dumping the symbol table */
    struct tree_list *next;
} *reused = NULL;

static JRB non_reusable;
static JRB reusable;


struct id *make_id(char *name)
{
    JRB tree;
    JRB entry;
    struct id *id;

    tree = *name == '.' ? reusable : non_reusable;
    entry = jrb_find_str(tree,name);
    if (entry)
	return jval_v(jrb_val(entry));
    id = alloc_type(struct id);
    id->name = stralloc(name);
    id->defined = id->used = id->global = 0;
    id->area = NULL;
    id->loc = current_loc;
    jrb_insert_str(tree,id->name,new_jval_v(id));
    return id;
}


static void scrap_reusable(void)
{
    JRB entry;
    struct tree_list *tree;

    jrb_traverse(entry,reusable) {
	const struct id *id = jval_v(jrb_val(entry));

	if (!id->defined && id->used)
	    lerrorf(&id->loc,"undefined identifier \"%s\"",id->name);
    }

    /*
     * We can't throw away the reusable labels now, because they may still be
     * referenced in an expression where they are mixed with non-reusable
     * labels which are not yet defined.
     */
    tree = alloc_type(struct tree_list);
    tree->tree = reusable;
    tree->next = reused;
    reused = tree;
    reusable = make_jrb();
}


void assign(struct id *id,struct op *value)
{
    if (*id->name != '.')
	scrap_reusable();
    if (id->defined)
	lerrorf(&current_loc,"redefining \"%s\" (first definition at %s:%d)",
	  id->name,get_file(&id->loc),get_line(&id->loc));
    id->defined = 1;
    id->value = value;
    id->loc = current_loc;
}


void id_init(void)
{
    non_reusable = make_jrb();
    reusable = make_jrb();
}


static void write_id(FILE *file,const struct id *id,int regular)
{
    unsigned long value;
    int digits;

    if (!id->defined)
	return;

    value = evaluate(id->value);
    if (!id->area && value > 0xffff)
	digits = 8;
    else
	digits = id->area && id->area->attributes & ATTR_RAM ? 3 : 4;
    fprintf(file,"%s %*s%0*lX  %c  %-*s %-*s %s:%d\n",
      id->area ? id->area->attributes & ATTR_RAM ?
      "RAM" : "ROM" : "EQU",
      8-digits,"",digits,value,
      regular && id->global ? 'G' : 'L',
      ID_PAD,id->name,
      AREA_PAD,id->area ? id->area->name : "-",
      get_file(&id->loc),get_line(&id->loc));
}


void write_ids(FILE *file)
{
    JRB entry;
    struct tree_list *walk;

    entry = jrb_first(reusable);
    for (walk = reused; walk; walk = walk->next)
	walk->entry = jrb_first(walk->tree);
    while (1) {
	const struct id *id;
	JRB *first;

	if (entry == jrb_nil(reusable))
	    id = NULL;
	else {
	    id = jval_v(jrb_val(entry));
	    first = &entry;
	}
	for (walk = reused; walk; walk = walk->next) {
	    struct id *this = jval_v(jrb_val(walk->entry));

	    if (walk->entry == jrb_nil(walk->tree))
		continue;
	    if (!id || strcmp(id->name,this->name) > 0) {
		id = this;
		first = &walk->entry;
	    }
	}
	if (!id)
	    break;
	write_id(file,id,0);
	*first = jrb_next(*first);
    }
    jrb_traverse(entry,non_reusable) {
	struct id *id = jval_v(jrb_val(entry));

	write_id(file,id,1);
    }
}



static void free_tree(JRB tree)
{
    JRB entry;

    jrb_traverse(entry,tree) {
	struct id *id = jval_v(jrb_val(entry));

	free(id->name);
	if (id->defined)
	    put_op(id->value);
	free(id);
    }
    jrb_free_tree(tree);
}


void id_cleanup(void)
{
    free_tree(non_reusable);
    free_tree(reusable);
    while (reused) {
	struct tree_list *next = reused->next;

	free_tree(reused->tree);
	free(reused);
	reused = next;
    }
}
