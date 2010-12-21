/*
 * id.h - Identifier handling
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>
#include <string.h>
#include "jrb.h"

#include "util.h"
#include "error.h"
#include "id.h"


static struct tree_list {
    JRB tree;
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
    id->defined = id->global = 0;
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

	if (!id->defined)
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
	lerrorf(&id->loc,"redefining \"%s\" (first definition at %s:%d)");
    id->defined = 1;
    id->value = value;
    id->loc = current_loc;
}


void id_init(void)
{
    non_reusable = make_jrb();
    reusable = make_jrb();
}


static void free_tree(JRB tree)
{
    JRB entry;

    jrb_traverse(entry,tree) {
	struct id *id = jval_v(jrb_val(entry));

	free(id->name);
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
