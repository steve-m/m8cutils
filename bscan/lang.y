%{
/*
 * lang.y - Boundary scanner configuration language
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>

#include "util.h"
#include "error.h"
#include "interact.h"

#include "value.h"
#include "test.h"


/* 100 levels should be sufficient. Insanity kicks in around 3 ... */
#define MAX_STACK 100


static struct stack {
    uint64_t defined;
    struct alternatives *alt;
    struct alternatives **next_alt;
} stack[MAX_STACK];

static int sp = 0;


static void compatibility(uint8_t *a,uint8_t e)
{
    if (((e & VALUE_0) && (*a & VALUE_1)) ||
      ((e & VALUE_1) && (*a & VALUE_0)))
	yyerror("opposing strong drives are now allowed");
    if ((e & VALUE_0R) && (*a & VALUE_1R)) {
	if (verbose)
	    yywarn("removing 1R driven by 0R");
	*a &= ~VALUE_1R;
    }
    if ((e & VALUE_0R) && (*a & VALUE_1R)) {
	if (verbose)
	    yywarn("removing 0R driven by 1R");
	*a &= ~VALUE_0R;
    }
    if ((e & VALUE_Z) && (*a & VALUE_Z)) {
	if (verbose)
	    yywarn("removing undriven Z");
	*a &= ~VALUE_Z;
    }
    if (!*a && verbose)
	yywarn("no allowed values left");
}


static uint8_t compatible(uint8_t set)
{
    uint8_t res;

    res = VALUE_0 | VALUE_0R | VALUE_Z | VALUE_1R | VALUE_1;
    if (set & VALUE_0)
	res &= ~VALUE_1;
    if (set & VALUE_1)
	res &= ~VALUE_0;
    return res;
}


%}


%union {
    uint64_t set;
    int num;
    struct choice *choice;
    struct {
	uint8_t allow;
	uint8_t external;
    } settings;
};


%token			ALLOW EXTERNAL Z R

%token	<num>		PORT BIT

%type	<num>		values value
%type	<set>		ports port bits bit_range
%type	<choice>	choices choice
%type	<settings>	settings

%%

rules:
    | rules rule
    | rules '{'
	{
	    struct alternatives *alt;

	    if (sp == MAX_STACK)
		yyerror("alternatives stack overflow");
	    alt = alloc_type(struct alternatives);
	    stack[sp].defined = defined;
	    stack[sp].alt = alt;
	    stack[sp].next_alt = next_alt;
	    sp++;
	}
      choices '}'
	{
	    struct alternatives *alt;
	    struct choice *walk;

	    sp--;
	    alt = stack[sp].alt;
	    next_alt = stack[sp].next_alt;
	    alt->defined = 0;
	    for (walk = $4; walk; walk = walk->next)
		alt->defined |= walk->defined;
	    alt->choices = $4;
	    alt->next = NULL;
	    *next_alt = alt;
	    next_alt = &alt->next;
	    defined |= alt->defined;
	}
    ;

choices:
    choice
	{
	    $$ = $1;
	}
    | choice '|' choices
	{
	    $1->next = $3;
	    $$ = $1;
	}
    ;

choice:
	{
	    $<choice>$ = alloc_type(struct choice);
	    $<choice>$->alternatives = NULL;
	    next_alt = &$<choice>$->alternatives;
	}
    rules
	{
	    $$ = $<choice>1;
	    $$->defined = defined & ~stack[sp-1].defined;
	    memcpy($$->allow,allow,sizeof(allow));
	    memcpy($$->external,external,sizeof(external));
	    $$->next = NULL;
	    defined = stack[sp-1].defined;
	}
    ;

rule:
    ports settings
	{
	    int i;

	    if (defined & $1)
		yyerror("port bit(s) already defined");
	    defined |= $1;
	    if (defined & 0x300) {
		yywarn("removing P1[0] and P1[1]");
		defined &= ~(uint64_t) 0x300;
	    }
	    for (i = 0; i != MAX_PINS; i++)
		if (($1 >> i) & 1) {
		    allow[i] = $2.allow;
		    external[i] = $2.external;
		}
	}
    ;

ports:
    port
	{
	    $$ = $1;
	}
    | ports ',' port
	{
	    if ($1 & $3)
		yyerror("duplicate port bit(s)");
	    $$ = $1 | $3;
	}
    ;

port:
    PORT
	{
	    $$ = (uint64_t) 0xff << ($1*8);
	}
    | PORT '[' bits ']'
	{
	    $$ = $3 << ($1*8);
	}
    | '(' ')'
	{
	    $$ = 0;
	}
    | '(' ports ')'
	{
	    $$ = $2;
	}
    ;

bits:
    bit_range
	{
	    $$ = $1;
	}
    | bits ',' bit_range
	{
	    if ($1 & $3)
		yyerror("duplicate port bit");
	    $$ = $1 | $3;
	}
    ;

bit_range:
    BIT
	{
	    $$ = 1 << $1;
	}
    | BIT ':' BIT
	{
	    if ($1 < $3)
		yyerror("upper bit below lower bit in [upper:lower]");
	    $$ = ((2 << $1)-1) & ~((1 << $3)-1);
	}
    | '(' ')'
	{
	    $$ = 0;
	}
    | '(' bits ')'
	{
	    $$ = $2;
	}
    ;

settings:
    ALLOW values EXTERNAL values
	{
	    $$.allow = $2;
	    $$.external = $4;
	    compatibility(&$$.allow,$$.external);
	}
    | ALLOW values
	{
	    $$.allow = $2;
	    $$.external = compatible($2);
	}
    | EXTERNAL values
	{
	    $$.allow = compatible($2);
	    $$.external = $2;
	}
    ;

values:
    value
	{
	    $$ = $1;
	}
    | values ',' value
	{
	    if ($1 & $3)
		yyerror("duplicate value");
	    $$ = $1 | $3;
	}
    ;

value:
    Z
	{
	    $$ = VALUE_Z;
	}
    | BIT
	{
	    if ($1 > 1)
		yyerror("value must be 0 or 1");
	    $$ = $1 ? VALUE_1 : VALUE_0;
	}
    | BIT R
	{
	    if ($1 > 1)
		yyerror("value must be 0 or 1");
	    $$ = $1 ? VALUE_1R : VALUE_0R;
	}
    | '(' ')'
	{
	    $$ = 0;
	}
    | '(' values ')'
	{
	    $$ = $2;
	}
    ;
