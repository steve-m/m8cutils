%{
/*
 * m8c.l - M8C assembler language
 * 
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "error.h"
#include "id.h"
#include "input.h"
#include "backslash.h"

#include "y.tab.h"


#if 0
#define DEBUG(fmt,...) fprintf(stderr,fmt,__VA_ARGS__)
#else
#define DEBUG(...)
#endif


extern int if_false;


int yylex(void);


#define DO(action) \
    do { \
	if (recording) \
	    macro_add(yytext); \
	else { \
	    if (!if_false) { \
		action; \
	    } \
	} \
    } while (0);


/*
 * We allow for some recursive macro expansion, but considering that memory
 * requirements will be O(n^2), we limit it to a relatively small value.
 */

#define MAX_FILE_DEPTH 300

struct {
    YY_BUFFER_STATE buffer;
	/* buffer being read before include/macro expansion */
    struct loc loc; /* location before include/macro expansion */
    struct macro *macro; /* macro expanding when we push */
} file_stack[MAX_FILE_DEPTH];

static int file_sp = 0;
static struct macro *macros = NULL;
static struct macro *recording = NULL; /* macro we're recording */
static struct macro *expanding = NULL; /* top macro we're expanding */
static int arguments = 0; /* non-zero if reading macro arguments */


/*
 * @@@ Macro lookup and recording are implemented in a pretty inefficient way.
 * Fix this if making heavy use of them.
 */


static struct macro *find_macro(const struct id *id)
{
    struct macro *walk;

    for (walk = macros; walk; walk = walk->next)
	if (!strcmp(walk->name,id->name))
	    break;
    return walk;
}


void begin_macro(const struct id *id)
{
    if (find_macro(id))
	yyerrorf("duplicate macro \"%s\"",id->name);
    recording = alloc_type(struct macro);
    recording->name = stralloc(id->name);
    recording->num_args = 0;
    recording->loc = current_loc;
    init_string(&recording->body);
}


void end_macro(void)
{
    const char *s;

    s = get_string(&recording->body);
    while (*s) {
	int n;

	s = strchr(s,'@');
	if (!s)
	    break;
	if (!isdigit(s[1]))
	    s++;
	else {
	    n = s[1]-'0';
	    if (n >= recording->num_args)
		recording->num_args = n+1;
	    s += 2;
	}
    }
    recording->next = macros;
    macros = recording;
    recording = NULL;
}


static void push_input(void)
{
    if (file_sp == MAX_FILE_DEPTH)
	yyerrorf("exceeded maximum depth of includes or macro expansions (%d)",
	  MAX_FILE_DEPTH);
    file_stack[file_sp].buffer = YY_CURRENT_BUFFER;
    file_stack[file_sp].loc = current_loc;
    file_stack[file_sp].macro = expanding;
    file_sp++;
}


static void add_to_arg(struct dyn_string *arg,const char *s)
{
    if (!*get_string(arg))
	while (*s && isspace(*s))
	    s++;
    add_string(arg,s);
}


static void trim_arg(struct dyn_string *arg)
{
    char *s;

    for (s = strchr(arg->s,0); s != arg->s && isspace(s[-1]); s--)
	s[-1] = 0;
}


void expand_macro(const struct id *id)
{
    struct macro *macro;
    struct dyn_string txt,arg[10];
    const char *s,*next;
    int i;

    macro = find_macro(id);
    if (!macro)
	yyerrorf("unrecognized mnemonic \"%s\"",id->name);
    if (macro->num_args) {
	int token;

	arguments = 1;
	for (i = 0; i != macro->num_args-1; i++) {
	    init_string(arg+i);
	    while (1) {
		token = yylex();
		if (token == ',')
		    break;
		add_to_arg(arg+i,yytext);
	    }
	    trim_arg(arg+i);
	}
	init_string(arg+i);
	while (1) {
	    token = yylex();
	    if (token == ',' || token == '\n')
		break;
	    add_to_arg(arg+i,yytext);
	}
	trim_arg(arg+i);
	/* skip any extra arguments */
	while (token != '\n')
	    token = yylex();
	arguments = 0;
    }
    push_input();
    current_loc = macro->loc;
    init_string(&txt);
    for (s = get_string(&macro->body); *s; s = next) {
	next = strchr(s,'@');
	if (!next) {
	    add_string(&txt,s);
	    break;
	}
	add_string_n(&txt,s,next-s);
	if (!isdigit(next[1])) {
	    add_string(&txt,"@");
	    next++;
	}
	else {
	    add_string(&txt,get_string(arg+(next[1]-'0')));
	    next += 2;
	}
    }
    for (i = 0; i != macro->num_args; i++)
	free_string(arg+i);
    yy_scan_string(get_string(&txt));
    DEBUG("EXPAND{%s}\n",get_string(&txt));
    free_string(&txt);
    expanding = macro;
}


static void macro_add(const char *s)
{
    add_string(&recording->body,s);
}


void remove_macros(void)
{
    while (macros) {
	struct macro *next;

	next = macros->next;
	free((char *) macros->name);
	free_string(&macros->body);
	free(macros);
	macros = next;
    }
}


void include_file(const char *name)
{
    push_input();
    yyin = fopen(name,"r");
    if (!yyin)
	yyerrorf("%s: %s",name,strerror(errno));
    set_file((char *) name); /* due t stupid jrb prototype */
    yy_switch_to_buffer(yy_create_buffer(yyin,YY_BUF_SIZE));
}


static void pop_input(void)
{
    int i;

    file_sp--;
    yy_delete_buffer(YY_CURRENT_BUFFER);
    current_loc = file_stack[file_sp].loc;
    expanding = file_stack[file_sp].macro;
    yy_switch_to_buffer(file_stack[file_sp].buffer);
}

%}


%%

[Aa]				DO(return TOK_A);
[Ff]				DO(return TOK_F);
[Ss][Pp]			DO(return TOK_SP);
[Xx]				DO(return TOK_X);
[Rr][Ee][Gg]			DO(return TOK_REG);

[Aa][Dd][Cc]			DO(return TOK_ADC);
[Aa][Dd][Dd]			DO(return TOK_ADD);
[Aa][Nn][Dd]			DO(return TOK_AND);
[Aa][Ss][Ll]			DO(return TOK_ASL);
[Aa][Ss][Rr]			DO(return TOK_ASR);
[Cc][Aa][Ll][Ll]		DO(return TOK_CALL);
[Cc][Mm][Pp]			DO(return TOK_CMP);
[Cc][Pp][Ll]			DO(return TOK_CPL);
[Dd][Ee][Cc]			DO(return TOK_DEC);
[Hh][Aa][Ll][Tt]		DO(return TOK_HALT);
[Ii][Nn][Cc]			DO(return TOK_INC);
[Ii][Nn][Dd][Ee][Xx]		DO(return TOK_INDEX);
[Jj][Aa][Cc][Cc]		DO(return TOK_JACC);
[Jj][Cc]			DO(return TOK_JC);
[Jj][Mm][Pp]			DO(return TOK_JMP);
[Jj][Nn][Cc]			DO(return TOK_JNC);
[Jj][Nn][Zz]			DO(return TOK_JNZ);
[Jj][Zz]			DO(return TOK_JZ);
[Ll][Cc][Aa][Ll][Ll]		DO(return TOK_LCALL);
[Ll][Jj][Mm][Pp]		DO(return TOK_LJMP);
[Mm][Oo][Vv]			DO(return TOK_MOV);
[Mm][Vv][Ii]			DO(return TOK_MVI);
[Nn][Oo][Pp]			DO(return TOK_NOP);
[Oo][Rr]			DO(return TOK_OR);
[Pp][Oo][Pp]			DO(return TOK_POP);
[Pp][Uu][Ss][Hh]		DO(return TOK_PUSH);
[Rr][Ee][Tt]			DO(return TOK_RET);
[Rr][Ee][Tt][Ii]		DO(return TOK_RETI);
[Rr][Ll][Cc]			DO(return TOK_RLC);
[Rr][Oo][Mm][Xx]		DO(return TOK_ROMX);
[Rr][Rr][Cc]			DO(return TOK_RRC);
[Ss][Bb][Bb]			DO(return TOK_SBB);
[Ss][Uu][Bb]			DO(return TOK_SUB);
[Ss][Ww][Aa][Pp]		DO(return TOK_SWAP);
[Ss][Ss][Cc]			DO(return TOK_SSC);
[Tt][Ss][Tt]			DO(return TOK_TST);
[Xx][Oo][Rr]			DO(return TOK_XOR);

[Aa][Rr][Ee][Aa]		DO(return TOK_AREA);
[Aa][Ss][Cc][Ii][Zz]		DO(return TOK_ASCIZ);
[Aa][Ss][Ss][Ee][Rr][Tt]	DO(return TOK_ASSERT);
[Bb][Ll][Kk]			DO(return TOK_BLK);
[Bb][Ll][Kk][Ww]		DO(return TOK_BLKW);
[Dd][Bb]			DO(return TOK_DB);
[Dd][Ss]			DO(return TOK_DS);
[Dd][Ss][Uu]			DO(return TOK_DSU);
[Dd][Ww]			DO(return TOK_DW);
[Dd][Ww][Ll]			DO(return TOK_DWL);
[Ee][Qq][Uu]			DO(return TOK_EQU);
[Ee][Xx][Pp][Oo][Rr][Tt]	DO(return TOK_EXPORT);
[Ii][Nn][Cc][Ll][Uu][Dd][Ee]	DO(return TOK_INCLUDE);
\.[Ll][Ii][Tt][Ee][Rr][Aa][Ll]	DO(return TOK_LITERAL);
\.[Ee][Nn][Dd][Ll][Ii][Tt][Ee][Rr][Aa][Ll] DO(return TOK_ENDLITERAL);
[Oo][Rr][Gg]			DO(return TOK_ORG);
\.[Ss][Ee][Cc][Tt][Ii][Oo][Nn]	DO(return TOK_SECTION);
\.[Ee][Nn][Dd][Ss][Ee][Cc][Tt][Ii][Oo][Nn] DO(return TOK_ENDSECTION);

[Mm][Aa][Cc][Rr][Oo]		{ if (recording)
				    yyerror("macros cannot define new macros");
				    DO(return TOK_MACRO); }
[Ee][Nn][Dd][Mm]		if (!if_false) return TOK_ENDM;

[Ii][Ff]			{ if (recording)
				    macro_add(yytext);
				  else
				    return if_false ? TOK_IF_FALSE : TOK_IF; }
[Ee][Ll][Ss][Ee]		{ if (recording)
				    macro_add(yytext);
				  else
				    return TOK_ELSE; }
[Ee][Nn][Dd][Ii][Ff]		{ if (recording)
				    macro_add(yytext);
				  else
				    return TOK_ENDIF; }

0[Xx][0-9A-Fa-f]+|[0-9]+	{ DO({
				  char *end;
				  yylval.num = strtoul(yytext,&end,0);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; })}
[0-9A-Fa-f]+[Hh]		{ DO({
				  yylval.num = strtoul(yytext,NULL,16);
				  return NUMBER; })}
$[0-9A-Fa-f]+			{ DO({
				  char *end;
				  yylval.num = strtoul(yytext+1,&end,16);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; })}
0b[01]+				{ DO({
				  char *end;
				  yylval.num = strtoul(yytext+2,&end,2);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; })}
%[01]+				{ DO({
				  char *end;
				  yylval.num = strtoul(yytext+1,&end,2);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; })}

\.?[a-zA-Z_][a-zA-Z_0-9]*	{ DO({
				  yylval.id = make_id(yytext);
				  return LABEL; })}
\.?[a-zA-Z_][a-zA-Z_0-9]*:	{ DO({
				  if (!arguments) {
				    *strchr(yytext,':') = 0;
				    yylval.id = make_id(yytext);
				  }
				  return LOCAL; })}
\.[a-zA-Z_][a-zA-Z_0-9]*::	{ DO(
				  yyerror("a reusable label cannot be global");
				  )}
[a-zA-Z_][a-zA-Z_0-9]*::	{ DO({
				  if (!arguments) {
				    *strchr(yytext,':') = 0;
				    yylval.id = make_id(yytext);
				  }
				  return GLOBAL; })}
\.[0-9][a-zA-Z_0-9]*[bB]	{ DO({
				  if (!arguments) {
				    yytext[strlen(yytext)-1] = 0;
				    yylval.id = make_id(yytext);
				  }
				  return BACKWARDS; })}
\.[0-9][a-zA-Z_0-9]*[fF]	{ DO({
				  if (!arguments) {
				    yytext[strlen(yytext)-1] = 0;
				    yylval.id = make_id(yytext);
				  }
				  return FORWARDS; })}
\.[0-9][a-zA-Z_0-9]*:		{ DO({
				  if (!arguments) {
				    *strrchr(yytext,':') = 0;
				    yylval.id = make_id(yytext);
				  }
				  return REDEFINABLE; })}

"||"				DO(return TOK_LOGICAL_OR);
&&				DO(return TOK_LOGICAL_AND);
"<<"				DO(return TOK_SHL);
">>"				DO(return TOK_SHR);
==				DO(return TOK_EQ);
!=				DO(return TOK_NE);
"<="				DO(return TOK_LE);
">="				DO(return TOK_GE);
"++"				DO(return TOK_DOUBLE_PLUS);

'[^']'				{ DO({
				    if (!isascii(yytext[1]))
				    yyerror("non-ASCII character constant");
				  yylval.num = yytext[1];
				  return NUMBER; })}
'\\''				{ yylval.num = '\'';
				  DO(return NUMBER); }
'\\\\'				{ yylval.num = '\\';
				  DO(return NUMBER); }
\"(\\[^\n\t]|[^\\"\n\t])*\"	{ DO({
				  if (!arguments) {
				    char *s;
				    *strrchr(yytext,'"') = 0;
				    s = stralloc(yytext+1);
				    backslash(s);
				    yylval.str = s;
				  }
				  return STRING; })}

[\t ]*               		{ if (recording)
				    macro_add(yytext); }
\n				{ if (recording)
				    macro_add(yytext);
				  current_loc.line++;
				  if (arguments)
				    return '\n'; }
;[^\n]*\n			{ if (recording)
				    macro_add(yytext);
				  current_loc.line++;
				  if (arguments)
				    return '\n'; }
"//"[^\n]*\n			{ if (recording)
				    macro_add(yytext);
				  current_loc.line++;
				  if (arguments)
				    return '\n'; }

^#\ [0-9]+\ \"[^"]*\"(\ [0-9]+)*\n {
				  char *start = strchr(yytext,'"')+1;
				  if (recording)
				    macro_add(yytext);
				  *strchr(start,'"') = 0;
				  set_file(start);
                                  current_loc.line = strtol(yytext+2,NULL,0);
				  if (arguments) {
				    *yytext = '\n';
				    return '\n';
				  } }

.				DO(return *yytext);

<<EOF>>				{ if (!file_sp) {
				    if (arguments)
					yyerror("EOF in macro arguments");
				    if (recording)
					yyerror("unterminated macro");
				    yyterminate();
				  }
				  pop_input(); }
