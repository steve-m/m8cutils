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
#include "sim.h"
#include "reg.h"
#include "m8c.h"
#include "registers.h"


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
	  (pc-rom) & 0xffff,a,f,pgmode,xio,cf,zf,gie,x,sp);
}

static void byte_check(const char *dst,uint32_t value)
{
    if (value > 255 && value < ~(uint32_t) 0x7f)
	yyerrorf("value %d too large for %s byte",value,dst);
}

%}


%union {
    uint32_t num;
    const char *str;
};


%token		TOK_A TOK_F TOK_SP TOK_X TOK_REG TOK_RAM TOK_ROM
%token		TOK_LOGICAL_OR TOK_LOGICAL_AND TOK_SHL TOK_SHR
%token		TOK_EQ TOK_NE TOK_LE TOK_GE

%token		TOK_RUN TOK_CONNECT TOK_DISCONNECT TOK_NL

%token	<num>	NUMBER PORT
%token	<str>	STRING

%type	<num>	byte_mask
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
    ;


drive_port:
    PORT byte_mask '=' expression
	{
	    uint8_t max;

	    max = $2 >> ctz($2);
	    if ($4 > max)
		yyerrorf("value %d too large for field (max. %d)",$4,$2);
	    gpio_drive($1,$2,$4 >> ctz(2));
	}
    ;


ice:
    TOK_CONNECT PORT byte_mask
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
    | TOK_DISCONNECT PORT byte_mask
	{
	    if (!ice)
		yyerror("there is no ICE");
	    gpio_ice_disconnect($2,$3);
	}
    ;


byte_mask:
	{
	    $$ = 0xff;
	}
    | '[' expression ']'
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


/* ----- Assignment -------------------------------------------------------- */


assignment:
    '[' NUMBER ']' '=' expression
	{
	    if ($2 > chip->pages*256)
		yyerrorf("address %u is outside of RAM",(unsigned) $2);
	    byte_check("RAM",$5);
	    ram[$2 >> 8][$2 & 255] = $5;
	}
    | TOK_RAM '[' NUMBER ']' '=' expression
	{
	    if ($3 > chip->pages*256)
		yyerrorf("address %u is outside of RAM",(unsigned) $3);
	    byte_check("RAM",$6);
	    ram[$3 >> 8][$3 & 255] = $6;
	}
    | TOK_REG '[' NUMBER ']' '=' expression
	{
	    void (*fn)(struct reg *reg,uint8_t value);

	    if ($3 > NUM_REGS)
		yyerrorf("address %u is outside of REG",(unsigned) $3);
	    byte_check("register",$6);
	    fn = regs[$3].ops->sim_write;
	    if (!fn)
		fn = regs[$3].ops->cpu_write;
	    if (!fn)
		yyerrorf("register 0x%03x (%s) is not writeable",$3,
		  regs[$3].name ? regs[$3].name : "unknown");
	    fn(regs+$3,$6);
	}
    | TOK_A '=' expression
	{
	    byte_check("A",$3);
	    a = $3;
	}
    | TOK_F '=' expression
	{
	    byte_check("F",$3);
	    regs[CPU_F].ops->sim_write(regs+CPU_F,$3);
	}
    | TOK_SP '=' expression
	{
	    byte_check("SP",$3);
	    sp = $3;
	}
    | TOK_X '=' expression
	{
	    byte_check("X",$3);
	    x = $3;
	}
    | '.' '=' expression
	{
	    if ($3 > 0xffff)
		yyerrorf("value 0x%x too large for PC",$3);
	    pc = rom+$3;
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
    | postfix_expression '[' expression ']'
	{
	    $$ = ($1 >> $3) & 1;
	}
    | postfix_expression '[' expression ':' expression ']'
	{
	    if ($3 < $5)
		yyerror("upper bit below lower bit in [upper:lower]");
	    $$ = ($1 >> $5) & ((2 << ($3-$5))-1);
	}
    ;

primary_expression:
    NUMBER
	{
	    $$ = $1;
	}
    | '[' NUMBER ']'
	{
	    if ($2 > chip->pages*256)
		yyerrorf("address %u is outside of RAM",(unsigned) $2);
	    $$ = ram[$2 >> 8][$2 & 255];
	}
    | TOK_RAM '[' NUMBER ']'
	{
	    if ($3 > chip->pages*256)
		yyerrorf("address %u is outside of RAM",(unsigned) $3);
	    $$ = ram[$3 >> 8][$3 & 255];
	}
    | TOK_ROM '[' NUMBER ']'
	{
	    if ($3 > chip->banks*chip->blocks*BLOCK_SIZE)
		yyerrorf("address %u is outside of ROM",(unsigned) $3);
	    $$ = rom[$3];
	}
    | TOK_REG '[' NUMBER ']'
	{
	    uint8_t (*fn)(struct reg *reg);

	    if ($3 > NUM_REGS)
		yyerrorf("address %u is outside of REG",(unsigned) $3);
	    fn = regs[$3].ops->sim_read;
	    if (!fn)
		fn = regs[$3].ops->cpu_read;
	    if (!fn)
		yyerrorf("register 0x%03x (%s) is not readable",$3,
		  regs[$3].name ? regs[$3].name : "unknown");
	    $$ = fn(regs+$3);
	}
    | TOK_A
	{
	    $$ = a;
	}
    | TOK_F
	{
	    $$ = f;
	}
    | TOK_SP
	{
	    $$ = sp;
	}
    | TOK_X
	{
	    $$ = x;
	}
    | '.'
	{
	    $$ = pc-rom;
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
