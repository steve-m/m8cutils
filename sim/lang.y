%{
/*
 * lang.y - Scripting language of the simulator
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "m8c.h"
#include "interact.h"
#include "chips.h"
#include "disasm.h"
#include "symmap.h"
#include "file.h"

#include "util.h"
#include "id.h"
#include "sim.h"
#include "reg.h"
#include "core.h"
#include "printf.h"
#include "lang.h"


static uint32_t now = 0;


static void status(void)
{
    char buf[MAX_DISASM_LINE];
    const char *sym;
    int size,i,attr;

    if (quiet)
	return;
    printf("%04x: A=%02x F=%02x (PgMode=%d XIO=%d Carry=%d "
      "Zero=%d GIE=%d) X=%02x SP=%02x\n",
      pc,a,f,pgmode,xio,cf,zf,gie,x,sp);
    sym = sym_by_value(pc,SYM_ATTR_ROM,&attr);
    if (sym)
	printf("%s:%s\n",sym,attr & SYM_ATTR_GLOBAL ? ":" : "");
    size = m8c_disassemble(pc,rom+pc,buf,sizeof(buf));
    printf("%04X ",pc);
    for (i = 0; i != size; i++)
	printf(" %02X",rom[pc+i]);
    printf("\t%s\n",buf);
}


static void run_cycles(uint32_t cycles)
{
    now += m8c_run(cycles);
    if (interrupted) {
	interrupted = 0;
	fflush(stdout);
	fprintf(stderr,"Stopped\n");
    }
    status();
}


static void run_one(void)
{
    now += m8c_step();
    status();
}


static uint32_t read_lvalue(const struct lvalue *lv)
{
    uint8_t (*fn)(struct reg *reg);
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
		yyerrorf("register 0x%03x (%s) is not readable",
		  (unsigned) lv->n,
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
	case lt_ice:
	    v = ice_read(lv->n);
	    break;
	default:
	    abort();
    }
    return (v & lv->mask) >> ctz(lv->mask);
}


static void write_lvalue(const struct lvalue *lv,uint32_t rvalue)
{
    void (*fn)(struct reg *reg,uint8_t value);

    rvalue = (rvalue << ctz(lv->mask)) & lv->mask;
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
		yyerrorf("register 0x%03x (%s) is not writeable",
		  (unsigned) lv->n,
		  regs[lv->n].name ? regs[lv->n].name : "unknown");
	    if (lv->mask == 0xff)
		fn(regs+lv->n,rvalue);
	    else {
		struct lvalue tmp_lv = *lv;

		tmp_lv.mask = 0xff;
		fn(regs+lv->n,(read_lvalue(&tmp_lv) & ~lv->mask) | rvalue);
	    }
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
	case lt_ice:
	    if (lv->mask == 0xff)
		ice_write(lv->n,rvalue);
	    else
		ice_write(lv->n,(ice_read(lv->n) & ~lv->mask) | rvalue);
	    break;
	default:
	    abort();
    }
}


%}


%union {
    uint32_t num;
    const char *str;
    struct id *id;
    struct lvalue lval;
    struct nlist *nlist;
};


%token		TOK_A TOK_F TOK_SP TOK_X TOK_REG TOK_RAM TOK_ROM
%token		TOK_LOGICAL_OR TOK_LOGICAL_AND TOK_SHL TOK_SHR
%token		TOK_EQ TOK_NE TOK_LE TOK_GE

%token		TOK_RUN TOK_STEP TOK_BREAK TOK_UNBREAK
%token		TOK_CONNECT TOK_DISCONNECT TOK_DRIVE TOK_SET
%token		TOK_DEFINE
%token		TOK_QUIT TOK_NL TOK_PRINTF TOK_SLEEP
%token		TOK_Z TOK_R TOK_ANALOG
%token		TOK_ICE

%token	<num>	NUMBER PORT
%token	<str>	STRING
%token	<id>	NEW_ID OLD_ID BACKWARDS FORWARDS

%type	<lval>	lvalue

%type	<num>	opt_byte_mask byte_mask register register_or_id opt_r
%type	<num>	expression logical_or_expression logical_and_expression
%type	<num>	low_expression high_expression
%type	<num>	inclusive_or_expression exclusive_or_expression and_expression
%type	<num>	equality_expression relational_expression
%type	<num>	shift_expression
%type	<num>	additive_expression multiplicative_expression
%type	<num>	unary_expression postfix_expression primary_expression

%type	<nlist>	expression_list

%nonassoc SINGLE_REG
%nonassoc ']'

%%


all:
    | all item
	{
	    fflush(stdout);
	}
    ;

item:
    TOK_NL
    | expression TOK_NL
	{
	    printf("0x%lx %lu\n",(unsigned long) $1,(unsigned long) $1);
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
		yyerrorf("can't turn back time (%lu < %lu)\n",
		  (unsigned long) $2,(unsigned long) now);
	    if ($2 != now)
		run_cycles($2-now);
	}
    | TOK_STEP
	{
	    run_one();
	}
    | TOK_DRIVE drive_port
    | TOK_SET set_port
    | PORT opt_byte_mask
	{
	    gpio_show(stdout,$1,$2);
	}
    | ice
    | define
    | printf
    | sleep
    | break
    | unbreak
    | TOK_QUIT
	{
	    exit(0);
	}
    ;

drive_port:
    PORT opt_byte_mask '=' expression opt_r
	{
	    if ($5)
		gpio_drive_r($1,$2,($4 << ctz($2)) & $2);
	    else
		gpio_drive($1,$2,($4 << ctz($2)) & $2);
	}
    | PORT opt_byte_mask '=' TOK_Z
	{
	    gpio_drive_z($1,$2);
	}
    ;

set_port:
    PORT opt_byte_mask '=' expression opt_r
	{
	    if ($5)
		gpio_set_r($1,$2,($4 << ctz($2)) & $2);
	    else
		gpio_set($1,$2,($4 << ctz($2)) & $2);
	}
    | PORT opt_byte_mask '=' TOK_Z
	{
	    gpio_set_z($1,$2);
	}
    | PORT opt_byte_mask '=' TOK_ANALOG
	{
	    gpio_set_analog($1,$2);
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
		yyerrorf("bit %ld out of range",(unsigned long) $2);
	    $$ = 1 << $2;
	}
    | '[' expression ':' expression ']'
	{
	    if ($2 < $4)
		yyerror("upper bit below lower bit in [upper:lower]");
	    if ($2 > 7)
		yyerrorf("bit %ld out of range",(unsigned long) $2);
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
    | TOK_DEFINE NEW_ID NUMBER
	{
	    id_value($2,$3);
	}
    ;

printf:
    TOK_PRINTF STRING expression_list
	{
	    do_printf(stdout,$2,$3);
	    free((char *) $2);
	    while ($3) {
		struct nlist *next = $3->next;

		free($3);
		$3 = next;
	    }
	}
    ;

expression_list:
	{
	    $$ = NULL;
	}
    | ',' expression expression_list
	{
	    $$ = alloc_type(struct nlist);
	    $$->n = $2;
	    $$->next = $3;
	}
    ;

sleep:
    TOK_SLEEP
	{
	    getchar();
	}
    | TOK_SLEEP expression
	{
	    sleep($2);
	}
    ;

break:
    TOK_BREAK
	{
	    m8c_break(pc);
	}
    | TOK_BREAK expression
	{
	    m8c_break($2);
	}
    | TOK_BREAK '*'
	{
	    m8c_break_show();
	}
    ;

unbreak:
    TOK_UNBREAK
	{
	    m8c_unbreak(pc);
	}
    | TOK_UNBREAK expression
	{
	    m8c_unbreak($2);
	}
    | TOK_UNBREAK '*'
	{
	    m8c_unbreak_all();
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
		yyerrorf("value 0x%lx too large for PC",(unsigned long) $3);
	    pc = $3;
	}
    ;

lvalue:
    opt_ram '[' expression ']' opt_byte_mask
	{
	    if ($3 > chip->pages*256)
		yyerrorf("address %lu is outside of RAM",(unsigned long) $3);
	    $$.type = lt_ram;
	    $$.n = $3;
	    $$.mask = $5;
	}
    | TOK_ROM '[' expression ']' opt_byte_mask
	{
	    if ($3 > chip->banks*chip->blocks*BLOCK_SIZE)
		yyerrorf("address %lu is outside of ROM",(unsigned long) $3);
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
    | TOK_ICE lvalue
	{
	    $$ = $2;
	    if ($$.type != lt_reg)
		yyerror(
		  "\"ice\" prefix is only allowed for register accesses");
	    if (!ice)
		yyerror("there is no ICE");
	    $$.type = lt_ice;
	}
    ;

opt_ram:
    | TOK_RAM
    ;

register:
    TOK_REG '[' expression ']' %prec SINGLE_REG
	{
	    if ($3 > NUM_REGS)
		yyerrorf("address %lu is outside of REG",(unsigned long) $3);
	    $$ = $3;
	}
    | TOK_REG '[' TOK_REG '[' expression ']' ']'
	{
	    if ($5 > NUM_REGS)
		yyerrorf("address %lu is outside of REG",(unsigned long) $5);
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
    | '&' opt_ram_rom_reg '[' expression ']'
	{
	    $$ = $4;
	}
    | '&' OLD_ID
	{
	    if ($2->field)
		yyerror("cannot obtain the address of a field");
	    $$ = $2->reg;
	}
    ;

opt_ram_rom_reg:
    | TOK_RAM
    | TOK_ROM
    | TOK_REG
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
    | NEW_ID
	{
	    const uint32_t *value;
	    int attr;

	    value = sym_by_name($1->name,SYM_ATTR_ANY,&attr,0,0);
	    if (!value)
		yyerrorf("no symbol \"%s\"",$1->name);
	    if (attr & SYM_ATTR_MORE)
		yyerrorf("symbol \"%s\" is not unique",$1->name);
	    $$ = *value;
	}
    | BACKWARDS
	{
	    const uint32_t *value;

	    value = sym_by_name($1->name,SYM_ATTR_ANY,NULL,pc,-1);
	    if (!value)
		yyerrorf("no symbol \"%s\"",$1->name);
	    $$ = *value;
	}
    | FORWARDS
	{
	    const uint32_t *value;

	    value = sym_by_name($1->name,SYM_ATTR_ANY,NULL,pc,1);
	    if (!value)
		yyerrorf("no symbol \"%s\"",$1->name);
	    $$ = *value;
	}
    | STRING
	{
	    const uint32_t *value;
	    int dir = 0,attr;

	    if (sym_is_redefinable($1)) {
		char *last = strchr($1,0)-1;

		switch (*last) {
		    case 'b':
		    case 'B':
			dir = -1;
			break;
		    case 'f':
		    case 'F':
			dir = 1;
			break;
		    default:
			yyerror("reference to re-definable label must end "
			  "with B or F");
		}
	    }
	    value = sym_by_name($1,SYM_ATTR_ANY,&attr,pc,dir);
	    if (!value)
		yyerrorf("no symbol \"%s\"",$1);
	    if (attr & SYM_ATTR_MORE)
		yyerrorf("symbol \"%s\" is not unique",$1);
	    $$ = *value;
	    free((char *) $1);
	}
    ;
