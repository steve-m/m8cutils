%{
/*
 * m8c.y - M8C assembler language
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "id.h"
#include "op.h"
#include "code.h"


#define STORE_OP_LOGIC(a,b,c,n) \
  store_op(((n) < 7 ? (a) : (n) < 9 ? (b)-7 : (c)-9)+(n))

int yyparse(void);

%}


%union {
    uint32_t num;
    const char *str;
    struct id *id;
    struct op *op;
};


%token		TOK_A TOK_F TOK_SP TOK_X TOK_REG
%token		TOK_ADC TOK_ADD TOK_AND TOK_ASL TOK_ASR
%token		TOK_CALL TOK_CMP TOK_CPL TOK_DEC TOK_HALT TOK_INC TOK_INDEX
%token		TOK_JACC TOK_JC TOK_JMP TOK_JNC TOK_JNZ TOK_JZ
%token		TOK_LCALL TOK_LJMP TOK_MOV TOK_MVI TOK_NOP TOK_OR
%token		TOK_POP TOK_PUSH TOK_RET TOK_RETI TOK_RLC TOK_ROMX TOK_RRC
%token		TOK_SBB TOK_SUB TOK_SWAP TOK_SSC TOK_TST TOK_XOR

%token		TOK_AREA TOK_ASCIZ TOK_BLK TOK_BLKW
%token		TOK_DB TOK_DS TOK_DSU TOK_DW TOK_DWL
%token		TOK_ELSE TOK_ENDIF TOK_ENDM TOK_EQU TOK_EXPORT
%token		TOK_IF TOK_INCLUDE TOK_LITERAL TOK_ENDLITERAL
%token		TOK_MACRO TOK_ORG TOK_SECTION TOK_ENDSECTION

%token		TOK_LOGICAL_OR TOK_LOGICAL_AND TOK_SHL TOK_SHR
%token		TOK_EQ TOK_NE TOK_LE TOK_GE
%token		TOK_DOUBLE_PLUS

%token	<num>	NUMBER
%token	<str>	STRING
%token	<id>	LABEL LOCAL GLOBAL

%type	<num>	arithmetic logic shift increment push
%type	<num>	area_attributes area_attribute

%type	<op>	index_expression label_directive
%type	<op>	expression logical_or_expression logical_and_expression
%type	<op>	low_expression high_expression
%type	<op>	inclusive_or_expression exclusive_or_expression and_expression
%type	<op>	equality_expression relational_expression
%type	<op>	shift_expression
%type	<op>	additive_expression multiplicative_expression
%type	<op>	unary_expression primary_expression
%%

all:
    | label
	{
	    next_statement();
	}
       all
    | statement
	{
	    next_statement();
	}
       all
    | directive
	{
	    next_statement();
	}
      all
    ;

/* @@@ change with adding relocation */
label:
    GLOBAL
	{
	    assign($1,number_op(*pc));
	    $1->global = 1;
	    $1->area = current_area;
	}
    | LOCAL
	{
	    assign($1,number_op(*pc));
	    $1->area = current_area;
	}
    | GLOBAL label_directive
	{
	    assign($1,$2);
	    $1->global = 1;
	}
    | LOCAL label_directive
	{
	    assign($1,$2);
	}
    ;

statement:
    TOK_ADC arithmetic
	{
	    store_op(9+$2);
	}
    | TOK_ADD arithmetic
	{
	    store_op(1+$2);
	}
    | TOK_ADD TOK_SP ',' expression
	{
	    store_op(0x38);
	    store(store_byte,1,$4);
	    advance_pc(2);
	}
    | TOK_AND logic
	{
	    STORE_OP_LOGIC(0x21,0x41,0x70,$2);
	}
    | TOK_ASL shift
	{
	    store_op(0x64+$2);
	}
    | TOK_ASR shift
	{
	    store_op(0x67+$2);
	}
    | TOK_CALL
	{
	    set_base(2);
	}
      pc_relative
	{
	    store_op(0x90);
	}
    | TOK_CMP cmp
    | TOK_CPL TOK_A
	{
	    store_op(0x73);
	    advance_pc(1);
	}
    | TOK_DEC increment
	{
	    store_op(0x78+$2);
	}
    | TOK_HALT
	{
	    store_op(0x30);
	    advance_pc(1);
	}
    | TOK_INC increment
	{
	    store_op(0x74+$2);
	}
    | TOK_INDEX
	{
	    set_base(2);
	}
      pc_relative
	{
	    store_op(0xf0);
	}
    | TOK_JACC
	{
	    set_base(1);
	}
      pc_relative
	{
	    store_op(0xe0);
	}
    | TOK_JC
	{
	    set_base(1);
	}
      pc_relative
	{
	    store_op(0xc0);
	}
    | TOK_JMP
	{
	    set_base(1);
	}
      pc_relative
	{
	    store_op(0x80);
	}
    | TOK_JNC
	{
	    set_base(1);
	}
      pc_relative
	{
	    store_op(0xd0);
	}
    | TOK_JNZ
	{
	    set_base(1);
	}
      pc_relative
	{
	    store_op(0xb0);
	}
    | TOK_JZ
	{
	    set_base(1);
	}
      pc_relative
	{
	    store_op(0xa0);
	}
    | TOK_LCALL pc_absolute
	{
	    store_op(0x7c);
	}
    | TOK_LJMP pc_absolute
	{
	    store_op(0x7d);
	}
    | TOK_MOV mov
    | TOK_MVI mvi
    | TOK_NOP
	{
	    store_op(0x40);
	    advance_pc(1);
	}
    | TOK_OR logic
	{
	    STORE_OP_LOGIC(0x29,0x43,0x71,$2);
	}
    | TOK_POP push
	{
	    store_op(0x18+$2);
	}
    | TOK_PUSH push
	{
	    store_op(0x8+$2);
	}
    | TOK_RET
	{
	    store_op(0x7f);
	    advance_pc(1);
	}
    | TOK_RETI
	{
	    store_op(0x7e);
	    advance_pc(1);
	}
    | TOK_RLC shift
	{
	    store_op(0x6a+$2);
	}
    | TOK_ROMX
	{
	    store_op(0x28);
	    advance_pc(1);
	}
    | TOK_RRC shift
	{
	    store_op(0x6d+$2);
	}
    | TOK_SBB arithmetic
	{
	    store_op(0x19+$2);
	}
    | TOK_SUB arithmetic
	{
	    store_op(0x11+$2);
	}
    | TOK_SWAP swap
    | TOK_SSC
	{
	    store_op(0);
	    advance_pc(1);
	}
    | TOK_TST tst
    | TOK_XOR logic
	{
	    STORE_OP_LOGIC(0x31,0x45,0x72,$2);
	}
    ;


/* ----- Instructions with an irregular argument pattern ------------------- */


cmp:
     TOK_A source_immediate
	{
	    store_op(0x39);
	}
     | TOK_A source_direct
	{
	    store_op(0x3a);
	}
     | TOK_A source_indexed
	{
	    store_op(0x3b);
	}
     | destination_direct_source_immediate
	{
	    store_op(0x3c);
	}
     | destination_indexed_source_immediate
	{
	    store_op(0x3d);
	}
     ;

mov:
     TOK_X ',' TOK_SP
	{
	    store_op(0x4f);
	    advance_pc(1);
	}
     | arithmetic
	{
	    store_op(0x50+$1);
	}
     | TOK_X source_immediate
	{
	    store_op(0x57);
	}
     | TOK_X source_direct
	{
	    store_op(0x58);
	}
     | TOK_X source_indexed
	{
	    store_op(0x59);
	}
     | destination_direct TOK_X
	{
	    store_op(0x5a);
	}
     | TOK_A ',' TOK_X
	{
	    store_op(0x5b);
	    advance_pc(1);
	}
     | TOK_X ',' TOK_A
	{
	    store_op(0x5c);
	    advance_pc(1);
	}
     | TOK_A ',' register_direct
	{
	    store_op(0x5d);
	}
     | TOK_A ',' register_indexed
	{
	    store_op(0x5e);
	}
     | '[' expression ']' ',' '[' expression ']'
	{
	    store_op(0x5f);
	    store(store_byte,1,$2);
	    store(store_byte,2,$6);
	    advance_pc(3);
	}
     | register_direct ',' TOK_A
	{
	    store_op(0x60);
	}
     | register_indexed ',' TOK_A
	{
	    store_op(0x61);
	}
     | register_direct ',' expression
	{
	    store_op(0x62);
	    store(store_byte,2,$3);
	    advance_pc(1);
	}
     | register_indexed ',' expression
	{
	    store_op(0x63);
	    store(store_byte,2,$3);
	    advance_pc(1);
	}
     ;

mvi:
     TOK_A source_direct
	{
	    store_op(0x3e);
	}
     | TOK_A ',' '[' '[' expression ']' TOK_DOUBLE_PLUS ']'
	{
	    store_op(0x3e);
	    store(store_byte,1,$5);
	    advance_pc(2);
	}
     | destination_direct TOK_A
	{
	    store_op(0x3f);
	}    
     | '[' '[' expression ']' TOK_DOUBLE_PLUS ']' ',' TOK_A
	{
	    store_op(0x3f);
	    store(store_byte,1,$3);
	    advance_pc(2);
	}
     ;

swap:
     TOK_A ',' TOK_X
	{
	    store_op(0x4b);
	    advance_pc(1);
	}
     | TOK_A source_direct
	{
	    store_op(0x4c);
	}
     | TOK_X source_direct
	{
	    store_op(0x4d);
	}
     | TOK_A ',' TOK_SP
	{
	    store_op(0x4e);
	    advance_pc(1);
	}
     ;

tst:
    destination_direct_source_immediate
	{
	    store_op(0x47);
	}
    | destination_indexed_source_immediate
	{
	    store_op(0x48);
	}
    | destination_direct_source_immediate_register
	{
	    store_op(0x49);
	}
    | destination_indexed_source_immediate_register
	{
	    store_op(0x4a);
	}
    ;

/* ----- Argument patterns ------------------------------------------------- */


arithmetic:
    TOK_A source_immediate
	{
	    $$ = 0;
	}
    | TOK_A source_direct
	{
	    $$ = 1;
	}
    | TOK_A source_indexed
	{
	    $$ = 2;
	}
    | destination_direct TOK_A
	{
	    $$ = 3;
	}
    | destination_indexed TOK_A
	{
	    $$ = 4;
	}
    | destination_direct_source_immediate
	{
	    $$ = 5;
	}
    | destination_indexed_source_immediate
	{
	    $$ = 6;
	}
    ;

logic:
    arithmetic
	{
	    $$ = $1;
	}
    | destination_direct_source_immediate_register
	{
	    $$ = 7;
	}
    | destination_indexed_source_immediate_register
	{
	    $$ = 8;
	}
    | TOK_F ',' expression
	{
	    store(store_byte,1,$3);
	    advance_pc(2);
	    $$ = 9;
	}
    ;

shift:
    TOK_A
	{
	    advance_pc(1);
	    $$ = 0;
	}
    | '[' expression ']'
	{
	    store(store_byte,1,$2);
	    advance_pc(2);
	    $$ = 1;
	}
    | index_expression
	{
	    store(store_byte,1,$1);
	    advance_pc(2);
	    $$ = 2;
	}
    ;

increment:
    shift
	{
	    $$ = $1 ? $1+1 : 0;
	}
    | TOK_X
	{
	    $$ = 1;
	    advance_pc(1);
	}
    ;

push:
    TOK_A
	{
	    $$ = 0;
	    advance_pc(1);
	}
    | TOK_X
	{
	    $$ = 8;
	    advance_pc(1);
	}
    ;

pc_relative:
    expression
	{
	    store(store_offset,0,$1);
	    advance_pc(2);
	}
    ;

pc_absolute:
    expression
	{
	    store(store_word,1,$1);
	    advance_pc(3);
	}
    ;


/* ----- Addressing modes -------------------------------------------------- */


source_immediate:
    ',' expression
	{
	    store(store_byte,1,$2);
	    advance_pc(2);
	}
    ;

source_direct:
    ',' '[' expression ']'
	{
	    store(store_byte,1,$3);
	    advance_pc(2);
	}
    ;

source_indexed:
    ',' index_expression
	{
	    store(store_byte,1,$2);
	    advance_pc(2);
	}
    ;

destination_direct:
    '[' expression ']' ','
	{
	    store(store_byte,1,$2);
	    advance_pc(2);
	}
    ;

destination_indexed:
    index_expression ','
	{
	    store(store_byte,1,$1);
	    advance_pc(2);
	}
    ;

destination_direct_source_immediate:
    '[' expression ']' ',' expression
	{
	    store(store_byte,1,$2);
	    store(store_byte,2,$5);
	    advance_pc(3);
	}
    ;

destination_indexed_source_immediate:
    index_expression ',' expression
	{
	    store(store_byte,1,$1);
	    store(store_byte,2,$3);
	    advance_pc(3);
	}
    ;

destination_direct_source_immediate_register:
    TOK_REG '[' expression ']' ',' expression
	{
	    store(store_byte,1,$3);
	    store(store_byte,2,$6);
	    advance_pc(3);
	}
    ;

destination_indexed_source_immediate_register:
    TOK_REG index_expression ',' expression
	{
	    store(store_byte,1,$2);
	    store(store_byte,2,$4);
	    advance_pc(3);
	}
    ;

register_direct:
    TOK_REG '[' expression ']'
	{
	    store(store_byte,1,$3);
	    advance_pc(2);
	}
    ;

register_indexed:
    TOK_REG index_expression
	{
	    store(store_byte,1,$2);
	    advance_pc(2);
	}
    ;


/* ----- Directives -------------------------------------------------------- */


label_directive:
    TOK_EQU expression
	{
	    $$ = $2;
	}
    ;

directive:
    TOK_AREA LABEL '(' area_attributes ')'
	{
	    set_area($2->name,$4);
	}
    | TOK_AREA LABEL
	{
	    set_area($2->name,0);
	}
    | TOK_ASCIZ STRING
	{
	    const char *s = $2;

	    check_store();
	    do {
		store_byte(&current_loc,*pc+s-$2,*s);
		advance_pc(1);
	    }
	    while (*s++);
	    free((char *) $2);
	}
    | TOK_BLK expression
	{
	    advance_pc(evaluate($2));
	    put_op($2);
	}
    | TOK_BLKW expression
	{
	    advance_pc(2*evaluate($2));
	    put_op($2);
	}
    | TOK_DB db
    | TOK_DW dw
    | TOK_DWL dwl
    | TOK_DS STRING
	{
	    const char *s = $2;

	    check_store();
	    while (*s) {
		store_byte(&current_loc,*pc+s-$2,*s);
		advance_pc(1);
		s++;
	    }
	    free((char *) $2);
	}
    | TOK_DSU STRING
	{
	    yyerror("not yet implemented");
	}
    | TOK_EXPORT LABEL
	{
	    if (*$2->name == '.')
		yyerror("reusable local labels cannot be exported");
	    /* @@@ silently ignore multiple exports of the same label ? */
	    $2->global = 1;
	}
    | TOK_IF
	{
	    yyerror("not yet implemented");
	}
    | TOK_INCLUDE
	{
	    yyerror("not yet implemented");
	}
    | TOK_LITERAL
    | TOK_ENDLITERAL
    | TOK_MACRO
	{
	    yyerror("not yet implemented");
	}
    | TOK_ORG expression
	{
	    next_pc = evaluate($2);
	    put_op($2);
	}
    | TOK_SECTION
    | TOK_ENDSECTION
    ;

area_attributes:
    area_attribute
	{
	    $$ = $1;
	}
    | area_attributes ',' area_attribute
	{
	    $$ = $1 | $3;
	}
    ;

area_attribute:
    LABEL
	{
	    if (!strcasecmp($1->name,"ram"))
		$$ = ATTR_RAM;
	    else if (!strcasecmp($1->name,"rom"))
		$$ = ATTR_ROM;
	    else if (!strcasecmp($1->name,"abs"))
		$$ = ATTR_ABS;
	    else if (!strcasecmp($1->name,"rel"))
		$$ = ATTR_REL;
	    else if (!strcasecmp($1->name,"con"))
		$$ = ATTR_CON;
	    else if (!strcasecmp($1->name,"ovr"))
		$$ = ATTR_OVR;
	    else
		yyerror("unrecognized area attribute");
	}
    ;

db:
    db_expression
    | db ',' db_expression
    ;

db_expression:
    expression
	{
	    store(store_byte,next_pc-*pc,$1);
	    advance_pc(1);
	}
    ;

dw:
    dw_expression
    | dw ',' dw_expression
    ;

dw_expression:
    expression
	{
	    store(store_word,next_pc-*pc,$1);
	    advance_pc(2);
	}
    ;

dwl:
    dwl_expression
    | dwl ',' dwl_expression
	{
	}
    ;

dwl_expression:
    expression
	{
	    store(store_word_le,next_pc-*pc,$1);
	    advance_pc(2);
	}
    ;


/* ----- Expressions ------------------------------------------------------- */


index_expression:
    '[' TOK_X '+' expression ']'
	{
	    $$ = $4;
	}
    | '[' TOK_X '-' expression ']'
	{
	    extension("[X-expr]");
	    $$ = make_op(op_minus,$4,NULL);
	}
    ;

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
	    extension("logical or");
	    $$ = make_op(op_logical_or,$1,$3);
	}
    ;

logical_and_expression:
    low_expression
	{
	    $$ = $1;
	}
    | logical_and_expression TOK_LOGICAL_AND low_expression
	{
	    extension("logical and");
	    $$ = make_op(op_logical_and,$1,$3);
	}
    ;

low_expression:
    high_expression
	{
	    $$ = $1;
	}
    | '<' high_expression
	{
	    $$ = make_op(op_low,$2,NULL);
	}
    ;

high_expression:
    inclusive_or_expression
	{
	    $$ = $1;
	}
    | '>' inclusive_or_expression
	{
	    $$ = make_op(op_high,$2,NULL);
	}
    ;

inclusive_or_expression:
    exclusive_or_expression
	{
	    $$ = $1;
	}
    | inclusive_or_expression '|' exclusive_or_expression
	{
	    $$ = make_op(op_or,$1,$3);
	}
    ;

exclusive_or_expression:
    and_expression
	{
	    $$ = $1;
	}
    | exclusive_or_expression '^' and_expression
	{
	    $$ = make_op(op_xor,$1,$3);
	}
    ;

and_expression:
    equality_expression
	{
	    $$ = $1;
	}
    | and_expression '&' equality_expression
	{
	    $$ = make_op(op_and,$1,$3);
	}
    ;

equality_expression:
    relational_expression
	{
	    $$ = $1;
	}
    | equality_expression TOK_EQ relational_expression
	{
	    extensions("relational operators");
	    $$ = make_op(op_eq,$1,$3);
	}
    | equality_expression TOK_NE relational_expression
	{
	    extensions("relational operators");
	    $$ = make_op(op_ne,$1,$3);
	}
    ;

relational_expression:
    shift_expression
	{
	    $$ = $1;
	}
    | relational_expression '<' shift_expression
	{
	    extensions("relational operators");
	    $$ = make_op(op_lt,$1,$3);
	}
    | relational_expression '>' shift_expression
	{
	    extensions("relational operators");
	    $$ = make_op(op_gt,$1,$3);
	}
    | relational_expression TOK_LE shift_expression
	{
	    extensions("relational operators");
	    $$ = make_op(op_le,$1,$3);
	}
    | relational_expression TOK_GE shift_expression
	{
	    extensions("relational operators");
	    $$ = make_op(op_ge,$1,$3);
	}
    ;

shift_expression:
    additive_expression
	{
	    $$ = $1;
	}
    | shift_expression TOK_SHL additive_expression
	{
	    extensions("shift operators");
	    $$ = make_op(op_shl,$1,$3);
	}
    | shift_expression TOK_SHR additive_expression
	{
	    extensions("shift operators");
	    $$ = make_op(op_shr,$1,$3);
	}
    ;

additive_expression:
    multiplicative_expression
	{
	    $$ = $1;
	}
    | additive_expression '+' multiplicative_expression
	{
	    $$ = make_op(op_add,$1,$3);
	}
    | additive_expression '-' multiplicative_expression
	{
	    $$ = make_op(op_sub,$1,$3);
	}
    ;

multiplicative_expression:
    unary_expression
	{
	    $$ = $1;
	}
    | multiplicative_expression '*' unary_expression
	{
	    $$ = make_op(op_mult,$1,$3);
	}
    | multiplicative_expression '/' unary_expression
	{
	    $$ = make_op(op_div,$1,$3);
	}
    | multiplicative_expression '%' unary_expression
	{
	    $$ = make_op(op_mod,$1,$3);
	}
    ;

unary_expression:
    primary_expression
	{
	    $$ = $1;
	}
    | '!' primary_expression
	{
	    $$ = make_op(op_logical_not,$2,NULL);
	}
    | '~' primary_expression
	{
	    $$ = make_op(op_not,$2,NULL);
	}
    | '-' primary_expression
	{
	    extension("unary minus");
	    $$ = make_op(op_minus,$2,NULL);
	}
    | TOK_SHR primary_expression
	{
	    extension("unary >>");
	    $$ = make_op(op_ctz,$2,NULL);
	}
    ;

primary_expression:
    NUMBER
	{
	    $$ = number_op($1);
	}
    | LABEL
	{
	    $$ = id_op($1);
	    $1->used = 1;
	}
    | '.'
	{
	    /* @@@ this changes when we add relocation */
	    $$ = number_op(*pc);
	}
    | '(' expression ')'
	{
	    $$ = $2;
	}
    ;
