/*
 * code.c - Code output
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdlib.h>

#include "util.h"
#include "error.h"
#include "op.h"
#include "code.h"


int pc = 0;
int highest_pc = 0;
int next_pc = 0;


static struct patch {
    void (*fn)(const struct loc *loc,int offset,uint32_t data);
    int pos;
    struct op *op;
    struct loc loc;
    struct patch *next;   
} *patches = NULL;



void advance_pc(int n)
{                             
    next_pc += n;                   
}


void next_statement(void)
{
    pc = next_pc;
    if (pc > highest_pc)
	highest_pc = pc;
}


void store_op(uint8_t op)
{
    program[pc] = op & 0x80 ? (program[pc] & 0xf) | op : op;
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
	lerrorf(loc,"offset .%+d is out of range %d to %d",
	    offset+base,base-2048,base+2047);
     program[pos] =
       (program[pos] & 0xf0) | ((offset >> 8) & 0xf);
     program[pos+1] = offset;
}


void set_base(int offset)
{
    program[pc+1] = offset;
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

    if (op->fn == op_number) {
	fn(&current_loc,pc+offset,op->u.value);
	put_op(op);
	return;
    }
    patch = alloc_type(struct patch);
    patch->fn = fn;
    patch->loc = current_loc;
    patch->pos = pc+offset;
    patch->op = op;
    patch->next = patches;
    patches = patch;
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
}
