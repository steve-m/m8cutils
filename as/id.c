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

static struct file_scope {
    JRB tree;
    JRB entry; /* for dumping the symbol table */
    struct file_scope *next;
} *file_scopes = NULL,*file_scope = NULL,**last_scope = &file_scopes;


static JRB global;
static JRB non_reusable;
static JRB reusable;


struct id *make_id(char *name)
{
    JRB tree;
    JRB entry;
    struct id *old = NULL,*id;

    tree = *name == '.' ? reusable : non_reusable;
    entry = jrb_find_str(tree,name);
    if (entry)
	return jval_v(jrb_val(entry));
    if (tree == non_reusable) {
	entry = jrb_find_str(global,name);
	if (entry)
	    old = jval_v(jrb_val(entry));
    }

    id = alloc_type(struct id);
    id->name = stralloc(name);
    id->defined = id->used = id->global = 0;
    id->area = NULL;
    id->loc = current_loc;
    id->alias = old;
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
	    lerrorf(&id->loc,"undefined reusable label \"%s\"",id->name);
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


void assign(struct id *id,struct op *value,const struct area *area)
{
    if (*id->name != '.')
	scrap_reusable();
    if (id->alias && id->global)
	id = id->alias;
    if (id->defined)
	lerrorf(&current_loc,"redefining \"%s\" (first definition at %s:%d)",
	  id->name,get_file(&id->loc),get_line(&id->loc));
    id->defined = 1;
    id->value = value;
    id->area = area;
    id->loc = current_loc;
}


void export(struct id *id)
{
    id->global = 1;
    if (!id->alias)
	return;
    id->alias->used = id->used || id->alias->used;
    if (id->alias->defined)
	lerrorf(&current_loc,"re-exporting \"%s\" (first definition at %s:%d)",
	  id->name,get_file(&id->alias->loc),get_line(&id->alias->loc));
    if (id->defined && !id->alias->defined) {
	id->alias->defined = 1;
	id->alias->value = get_op(id->value);
	id->alias->area = id->area;
	id->alias->loc = id->loc;
    }
}


struct op *id_resolve(const struct id *id)
{
    if (id->alias && id->global)
	id = id->alias;
    return id->defined ? id->value : NULL;
}


void id_init(void)
{
    global = make_jrb();
    non_reusable = make_jrb();
    reusable = make_jrb();
}


void id_begin_file(void)
{
    file_scope = alloc_type(struct file_scope);
    file_scope->tree = make_jrb();
    *last_scope = file_scope;
    file_scope->next = NULL;
    last_scope = &file_scope->next;
}


void id_end_file(void)
{
    JRB entry,next;

    for (entry = jrb_first(non_reusable); entry != jrb_nil(non_reusable);
      entry = next) {
	struct id *id = jval_v(jrb_val(entry));

	if (id->global && !id->defined && !(id->alias && id->alias->defined))
	    lerrorf(&id->loc,"undefined exported label \"%s\"",id->name);
	next = jrb_next(entry);
	jrb_delete_node(entry);
	if (!id->global && id->defined)
	    jrb_insert_str(file_scope->tree,id->name,new_jval_v(id));
	else {
	    id->global = 1;
	    jrb_insert_str(global,id->name,new_jval_v(id));
	}
    }
    assert(jrb_empty(non_reusable));
}


static void write_id(FILE *file,const struct id *id,int regular)
{
    unsigned long value;
    int digits;

    if (!id->defined || (id->alias && id->global))
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


#define WRITE_SORTED(unlist,list,regular) \
    { \
	JRB entry; \
	typeof(list) walk; \
	\
	entry = jrb_first(unlist); \
	for (walk = list; walk; walk = walk->next) \
	    walk->entry = jrb_first(walk->tree); \
	while (1) { \
	    const struct id *id; \
	    JRB *first; \
	    \
	    if (entry == jrb_nil(unlist)) \
		id = NULL; \
	    else { \
		id = jval_v(jrb_val(entry)); \
		first = &entry; \
	    } \
	    for (walk = list; walk; walk = walk->next) { \
		struct id *this = jval_v(jrb_val(walk->entry)); \
		\
		if (walk->entry == jrb_nil(walk->tree)) \
		    continue; \
		if (!id || strcmp(id->name,this->name) > 0) { \
		    id = this; \
		    first = &walk->entry; \
		} \
	    } \
	    if (!id) \
		break; \
	    write_id(file,id,regular); \
	    *first = jrb_next(*first); \
	} \
    }


void write_ids(FILE *file)
{
    WRITE_SORTED(reusable,reused,0);
    WRITE_SORTED(global,file_scopes,1);
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
    while (file_scopes) {
	struct file_scope *next = file_scopes->next;

	free_tree(file_scopes->tree);
	free(file_scopes);
	file_scopes = next;
    }
    free_tree(reusable);
    while (reused) {
	struct tree_list *next = reused->next;

	free_tree(reused->tree);
	free(reused);
	reused = next;
    }
}
