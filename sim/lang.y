%{
/*
 * lang.y - Scripting language of the simulator
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdio.h>

#include "interact.h"
#include "chips.h"
#include "file.h"

#include "util.h"
#include "id.h"
#include "sim.h"
#include "reg.h"
#include "m8c.h"
#include "registers.h"
#include "lang.h"


static uint32_t now = 0;


static void run_cycles(uint32_t cycles)
{
    uint32_t done;

    done = m8c_run(cycles);
    if (done != cycles)
	printf("HALT\n");
    now += done;
    if (!quiet)
	printf("%04x: A=%02x F=%02x (PgMode=%d XIO=%d Carry=%d "
	  "Zero=%d GIE=%d) X=%02x SP=%02x\n",
	  pc,a,f,pgmode,xio,cf,zf,gie,x,sp);
}


static uint32_t read_lvalue(const struct lvalue *lv)
{
    uint8_t (*fn)(struct regs *reg);
    uint8_t v;

    switch (lv->type) {
	case lt_ram:
	    v = ram[lv->n];
	    break;
	case lt_rom:
	    v = rom[lv->n];
	    break;
	case lt_reg:
	    fn = regs[lv->n].ops->sim_read;
	    if (!fn)
		fn = regs[lv->n].ops->cpu_read;
	    if (!fn)
		yyerrorf("register 0x%03x (%s) is not readable",lv->n,
		  regs[lv->n].name ? regs[lv->n].name : "unknown");
	    v = fn(regs+lv->n);
	    break;
	case lt_a:
	    v = a;
	    break;
	case lt_f:
	    v = f;
	    break;
	case lt_sp:
	    v = sp;
	    break;
	case lt_x:
	    v = x;
	    break;
	default:
	    abort();
    }
    return (v & lv->mask) >> ctz(lv->mask);
}


static void write_lvalue(const struct lvalue *lv,uint8_t rvalue)
{
    void (*fn)(struct regs *reg,uint8_t value);

    if (lv->mask == 0xff) {
	if (rvalue > 255 && rvalue < ~(uint32_t) 0x7f)
	    yyerrorf("value %d too large for byte",rvalue);
    }
    else {
	if (rvalue > lv->mask)
	    yyerrorf("value %d too large for mask 0x%x",rvalue,lv->mask);
    }
    rvalue <<= ctz(lv->mask);
    switch (lv->type) {
	case lt_ram:
	    ram[lv->n] = (ram[lv->n] & ~lv->mask) | rvalue;
	    break;
	case lt_rom:
	    rom[lv->n] = (rom[lv->n] & ~lv->mask) | rvalue;
	    break;
	case lt_reg:
	    fn = regs[lv->n].ops->sim_write;
	    if (!fn)
		fn = regs[lv->n].ops->cpu_write;
	    if (!fn)
		yyerrorf("register 0x%03x (%s) is not writeable",lv->n,
		  regs[lv->n].name ? regs[lv->n].name : "unknown");
	    if (lv->mask == 0xff)
		fn(regs+lv->n,rvalue);
	    else
		fn(regs+lv->n,(read_lvalue(lv) & ~lv->mask) | rvalue);
	    break;
	case lt_a:
	    a = (a & ~lv->mask) | rvalue;
	    break;
	case lt_f:
	    regs[CPU_F].ops->sim_write(regs+CPU_F,(f & ~lv->mask) | rvalue);
	    break;
	case lt_sp:
	    sp = (sp & ~lv->mask) | rvalue;
	    break;
	case lt_x:
	    x = (x & ~lv->mask) | rvalue;
	    break;
	default:
	    abort();
    }
}


%}


%union {
    uint32_t num;
    struct id *id;
    struct lvalue lval;
};


%token		TOK_A TOK_F TOK_SP TOK_X TOK_REG TOK_RAM TOK_ROM
%token		TOK_LOGICAL_OR TOK_LOGICAL_AND TOK_SHL TOK_SHR
%token		TOK_EQ TOK_NE TOK_LE TOK_GE

%token		TOK_RUN TOK_CONNECT TOK_DISCONNECT TOK_DEFINE TOK_NL
%token		TOK_Z TOK_R

%token	<num>	NUMBER PORT
%token	<id>	NEW_ID OLD_ID

%type	<lval>	lvalue

%type	<num>	opt_byte_mask byte_mask register register_or_id opt_r
%type	<num>	expression logical_or_expression logical_and_expression
%type	<num>	low_expression high_expression
%type	<num>	inclusive_or_expression exclusive_or_expression and_expression
%type	<num>	equality_expression relational_expression
%type	<num>	shift_expression
%type	<num>	additive_expression multiplicative_expression
%type	<num>	unary_expression postfix_expression primary_expression

%%


all:
    | item all
    ;

item:
    TOK_NL
    | expression TOK_NL
	{
	    printf("0x%x %u\n",$1,$1);
	}
    | assignment TOK_NL
    | command TOK_NL
    ;


/* ----- Commands ---------------------------------------------------------- */


command:
    TOK_RUN
	{
	    run_cycles(0);
	}
    | TOK_RUN expression
	{
	    if ($2 < now)
		yyerrorf("can't turn back time (%u < %u)\n",$2,now);
	    if ($2 != now)
		run_cycles($2-now);
	}
    | drive_port
    | ice
    | define
    ;

drive_port:
    PORT opt_byte_mask '=' expression opt_r
	{
	    uint8_t max;

	    max = $2 >> ctz($2);
	    if ($4 > max)
		yyerrorf("value %d too large for field (max. %d)",$4,$2);
	    if ($5)
		gpio_drive_r($1,$2,$2);
	    gpio_drive($1,$2,$4 << ctz($2));
	    if (!$5)
		gpio_drive_r($1,$2,0);
	    gpio_drive_z($1,$2,0);
	}
    | PORT opt_byte_mask '=' TOK_Z
	{
	    gpio_drive_z($1,$2,$2);
	}
    ;

opt_r:
	{
	    $$ = 0;
	}
    | TOK_R
	{
	    $$ = 1;
	}
    ;

ice:
    TOK_CONNECT PORT opt_byte_mask
	{
	    if (!ice)
		yyerror("there is no ICE");
	    gpio_ice_connect($2,$3);
	}
    | TOK_DISCONNECT
	{
	    int i;

	    if (!ice)
		yyerror("there is no ICE");
	    for (i = 0; i != 8; i++)
		gpio_ice_disconnect(i,0xff);
	}
    | TOK_DISCONNECT PORT opt_byte_mask
	{
	    if (!ice)
		yyerror("there is no ICE");
	    gpio_ice_disconnect($2,$3);
	}
    ;

opt_byte_mask:
	{
	    $$ = 0xff;
	}
    | byte_mask
	{
	    $$ = $1;
	}
    ;

byte_mask:
    '[' expression ']'
	{
	    if ($2 > 7)
		yyerrorf("bit %d out of range",$2);
	    $$ = 1 << $2;
	}
    | '[' expression ':' expression ']'
	{
	    if ($2 < $4)
		yyerror("upper bit below lower bit in [upper:lower]");
	    if ($2 > 7)
		yyerrorf("bit %d out of range",$2);
	    $$ = ((2 << $2)-1) & ~((1 << $4)-1);
	}
    ;

define:
    TOK_DEFINE NEW_ID register_or_id
	{
	    id_reg($2,$3);
	}
    | TOK_DEFINE NEW_ID register_or_id byte_mask
	{
	    id_field($2,$3,$4);
	}
    ;

register_or_id:
    register
	{
	    $$ = $1;
	}
    | OLD_ID
	{
            if ($1->field)
		yyerror("must be a register, not a field");
	    $$ = $1->reg;
	}
    ;


/* ----- Assignment -------------------------------------------------------- */


assignment:
    lvalue '=' expression
	{
	    write_lvalue(&$1,$3);
	}
    | '.' '=' expression
	{
	    if ($3 > 0xffff)
		yyerrorf("value 0x%x too large for PC",$3);
	    pc = $3;
	}
    ;

lvalue:
    '[' NUMBER ']' opt_byte_mask
	{
	    if ($2 > chip->pages*256)
		yyerrorf("address %u is outside of RAM",(unsigned) $2);
	    $$.type = lt_ram;
	    $$.n = $2;
	    $$.mask = $4;
	}
    | TOK_RAM '[' NUMBER ']' opt_byte_mask
	{
	    if ($3 > chip->pages*256)
		yyerrorf("address %u is outside of RAM",(unsigned) $3);
	    $$.type = lt_ram;
	    $$.n = $3;
	    $$.mask = $5;
	}
    | TOK_ROM '[' NUMBER ']' opt_byte_mask
	{
	    if ($3 > chip->banks*chip->blocks*BLOCK_SIZE)
		yyerrorf("address %u is outside of ROM",(unsigned) $3);
	    $$.type = lt_rom;
	    $$.n = $3;
	    $$.mask = $5;
	}
    | register opt_byte_mask
	{
	    $$.type = lt_reg;
	    $$.n = $1;
	    $$.mask = $2;
	}
    | TOK_A opt_byte_mask
	{
	    $$.type = lt_a;
	    $$.mask = $2;
	}
    | TOK_F opt_byte_mask
	{
	    $$.type = lt_f;
	    $$.mask = $2;
	}
    | TOK_SP opt_byte_mask
	{
	    $$.type = lt_sp;
	    $$.mask = $2;
	}
    | TOK_X opt_byte_mask
	{
	    $$.type = lt_x;
	    $$.mask = $2;
	}
    | OLD_ID
	{
	    if (!$1->field) {
		$$.type = lt_reg;
		$$.n = $1->reg;
		$$.mask = 0xff;
	    }
	    else {
		$$.type = lt_reg;
		$$.n = $1->reg;
		$$.mask = $1->field;
	    }
	}
    | OLD_ID byte_mask
	{
	    if ($1->field)
		yyerror("masks are not allowed on fields");
	    $$.type = lt_reg;
	    $$.n = $1->reg;
	    $$.mask = $2;
	}
    ;

register:
    TOK_REG '[' NUMBER ']'
	{
	    if ($3 > NUM_REGS)
		yyerrorf("address %u is outside of REG",(unsigned) $3);
	    $$ = $3;
	}
    | TOK_REG '[' TOK_REG '[' NUMBER ']' ']'
	{
	    if ($5 > NUM_REGS)
		yyerrorf("address %u is outside of REG",(unsigned) $5);
	    $$ = $5;
	}
    ;


/* ----- Expressions ------------------------------------------------------- */


expression:
    logical_or_expression
	{
	    $$ = $1;
	}
    ;

logical_or_expression:
    logical_and_expression
	{
	    $$ = $1;
	}
    | logical_or_expression TOK_LOGICAL_OR logical_and_expression
	{
	    $$ = $1 || $3;
	}
    ;

logical_and_expression:
    low_expression
	{
	    $$ = $1;
	}
    | logical_and_expression TOK_LOGICAL_AND low_expression
	{
	    $$ = $1 && $3;
	}
    ;

low_expression:
    high_expression
	{
	    $$ = $1;
	}
    | '<' high_expression
	{
	    $$ = $2 & 0xff;
	}
    ;

high_expression:
    inclusive_or_expression
	{
	    $$ = $1;
	}
    | '>' inclusive_or_expression
	{
	    $$ = ($2 >> 8) & 0xff;
	}
    ;

inclusive_or_expression:
    exclusive_or_expression
	{
	    $$ = $1;
	}
    | inclusive_or_expression '|' exclusive_or_expression
	{
	    $$ = $1 | $3;
	}
    ;

exclusive_or_expression:
    and_expression
	{
	    $$ = $1;
	}
    | exclusive_or_expression '^' and_expression
	{
	    $$ = $1 ^ $3;
	}
    ;

and_expression:
    equality_expression
	{
	    $$ = $1;
	}
    | and_expression '&' equality_expression
	{
	    $$ = $1 & $3;
	}
    ;

equality_expression:
    relational_expression
	{
	    $$ = $1;
	}
    | equality_expression TOK_EQ relational_expression
	{
	    $$ = $1 == $3;
	}
    | equality_expression TOK_NE relational_expression
	{
	    $$ = $1 != $3;
	}
    ;

relational_expression:
    shift_expression
	{
	    $$ = $1;
	}
    | relational_expression '<' shift_expression
	{
	    $$ = $1 < $3;
	}
    | relational_expression '>' shift_expression
	{
	    $$ = $1 > $3;
	}
    | relational_expression TOK_LE shift_expression
	{
	    $$ = $1 <= $3;
	}
    | relational_expression TOK_GE shift_expression
	{
	    $$ = $1 >= $3;
	}
    ;

shift_expression:
    additive_expression
	{
	    $$ = $1;
	}
    | shift_expression TOK_SHL additive_expression
	{
	    $$ = $1 << $3;
	}
    | shift_expression TOK_SHR additive_expression
	{
	    $$ = $1 >> $3;
	}
    ;

additive_expression:
    multiplicative_expression
	{
	    $$ = $1;
	}
    | additive_expression '+' multiplicative_expression
	{
	    $$ = $1+$3;
	}
    | additive_expression '-' multiplicative_expression
	{
	    $$ = $1-$3;
	}
    ;

multiplicative_expression:
    unary_expression
	{
	    $$ = $1;
	}
    | multiplicative_expression '*' unary_expression
	{
	    $$ = $1*$3;
	}
    | multiplicative_expression '/' unary_expression
	{
	    $$ = $1/$3;
	}
    | multiplicative_expression '%' unary_expression
	{
	    $$ = $1 % $3;
	}
    ;

unary_expression:
    postfix_expression
	{
	    $$ = $1;
	}
    | '!' postfix_expression
	{
	    $$ = !$2;
	}
    | '~' postfix_expression
	{
	    $$ = ~$2;
	}
    | '-' postfix_expression
	{
	    $$ = -$2;
	}
    | TOK_SHR postfix_expression
	{
	    $$ = ctz($2);
	}
    | '&' '[' expression ']'
	{
	    $$ = $3;
	}
    | '&' TOK_REG '[' expression ']'
	{
	    $$ = $4;
	}
    ;

postfix_expression:
    primary_expression
	{
	    $$ = $1;
	}
    ;

primary_expression:
    NUMBER
	{
	    $$ = $1;
	}
    | lvalue
	{
	    $$ = read_lvalue(&$1);
	}
    | '.'
	{
	    $$ = pc;
	}
    | '@'
	{
	    $$ = now;
	}
    | '(' expression ')'
	{
	    $$ = $2;
	}
    ;
