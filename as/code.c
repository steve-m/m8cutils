/*
 * code.c - Code output
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>

#include "jrb.h"
#include "file.h"
#include "disasm.h"

#include "util.h"
#include "error.h"
#include "op.h"
#include "code.h"


static struct patch {
    void (*fn)(const struct loc *loc,int offset,uint32_t data);
    int pos;
    struct op *op;
    struct loc loc;
    struct patch *next;   
} *patches = NULL,**last_patch = &patches;

static struct assertion {
    struct loc loc;
    struct op *op;
    struct assertion *next;
} *assertions = NULL,**next_assertion = &assertions;


int *pc; /* must call set_area before doing anything else */
int next_pc = 0;
int cycles = 0;

const struct area *text;
struct area *current_area;


static JRB areas;


void advance_pc(int n)
{                             
    next_pc += n;                   
}


void next_statement(void)
{
    *pc = next_pc;
    if (*pc > current_area->highest_pc)
	current_area->highest_pc = *pc;
}


void check_store(void)
{
    if (current_area != text)
	yyerror("assembler can store data only in \"text\" area");
}


void store_op(uint8_t op)
{
    check_store();
    program[*pc] = op & 0x80 ? (program[*pc] & 0xf) | op : op;
    cycles += m8c_cycles[op];
}


void store_byte(const struct loc *loc,int pos,uint32_t data)
{
    program[pos] = data;
}


void store_word(const struct loc *loc,int pos,uint32_t data)
{
    program[pos] = data >> 8;
    program[pos+1] = data;
}


void store_word_le(const struct loc *loc,int pos,uint32_t data)
{
    program[pos] = data;
    program[pos+1] = data >> 8;
}


void store_offset(const struct loc *loc,int pos,uint32_t data)
{
     int base = program[pos+1];
     uint32_t offset;

     offset = (int16_t) (data-pos-base);
     if (offset > 0x7ff && offset < (uint32_t) -0x800)
	lerrorf(loc,"offset .%+ld is out of range %d to %d",
	    (unsigned long) (offset+base),base-2048,base+2047);
     program[pos] =
       (program[pos] & 0xf0) | ((offset >> 8) & 0xf);
     program[pos+1] = offset;
}


void set_base(int offset)
{
    program[*pc+1] = offset;
#if 0
    if (program[pc+1] < offset)
	program[pc] = (program[pc] & 0xf0) | ((program[pc]-1) & 0xf);
    program[pc+1] -= offset;
#endif
}


void store(void (*fn)(const struct loc *loc,int pos,uint32_t data),
  int offset,struct op *op)
{
    struct patch *patch;

    check_store();
    if (op->fn == op_number) {
	fn(&current_loc,*pc+offset,op->u.value);
	put_op(op);
	return;
    }
    patch = alloc_type(struct patch);
    patch->fn = fn;
    patch->loc = current_loc;
    patch->pos = *pc+offset;
    patch->op = op;
    patch->next = NULL;
    *last_patch = patch;
    last_patch = &patch->next;
}


void assertion(const struct loc *loc,struct op *op)
{
    struct assertion *assrt;

    assrt = alloc_type(struct assertion);
    assrt->loc = *loc;
    assrt->op = op;
    assrt->next = NULL;
    *next_assertion = assrt;
    next_assertion = &assrt->next;
}


void resolve(void)
{
    while (patches) {
	struct patch *next;

	patches->fn(&patches->loc,patches->pos,evaluate(patches->op));
	put_op(patches->op);
	next = patches->next;
	free(patches);
	patches = next;
    }
    while (assertions) {
	struct assertion *next;

	if (!evaluate(assertions->op))
	    lerrorf(&assertions->loc,"assertion failed");
	next = assertions->next;
	free(assertions);
	assertions = next;
    }
}


struct area *set_area(char *name,int attributes)
{
    JRB entry;
    struct area *area;

    entry = jrb_find_str(areas,name);
    if (entry) {
	area = jval_v(jrb_val(entry));
	if (attributes & ~area->attributes)
	    lerrorf(&current_loc,
	      "attributes of area differ from definition at %s:%d",
	      get_file(&area->loc),get_line(&area->loc));
    }
    else {
	if ((attributes & ATTR_RAM) && (attributes & ATTR_ROM))
	    yyerror("an area can't be both ROM and RAM");
	if (!(attributes & ATTR_RAM) && !(attributes & ATTR_ROM))
	    yyerror("must specify RAM or ROM");           
	if (attributes & ATTR_REL)
	    yyerror("relocatable areas are not yet supported");
	if ((attributes & ATTR_CON) && (attributes & ATTR_OVR))           
	    yyerror("an area can't be both CON and OVR");
	area = alloc_type(struct area);
	area->name = name;
	area->attributes = attributes;
	area->pc = area->highest_pc = 0;
	area->loc = current_loc;
	jrb_insert_str(areas,area->name,new_jval_v(area));
    }
    pc = &area->pc;
    if (area->attributes & ATTR_OVR)
	*pc = 0;
    next_pc = *pc;
    current_area = area;
    return area;
}



void code_init(void)
{
    areas = make_jrb();
    text = set_area("text",ATTR_ROM | ATTR_ABS | ATTR_CON);
}


void code_cleanup(void)
{
    JRB entry;

    jrb_traverse(entry,areas) {
	struct area *area = jval_v(jrb_val(entry));

	free(area);
    }
    jrb_free_tree(areas);
}
