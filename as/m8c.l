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

#include "util.h"
#include "error.h"
#include "id.h"
#include "macro.h"
#include "backslash.h"

#include "y.tab.h"


extern int if_false;


#define DO(action) \
    if (!if_false) { action; }


static int recording = 0;


void begin_macro(const struct id *id)
{
}


void end_macro(void)
{
}


void expand_macro(const struct id *id)
{
    yyerror("unrecognized mnemonic");
}


static void macro_add(void *x)
{
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

[Mm][Aa][Cc][Rr][Oo]		DO(return TOK_MACRO);
[Ee][Nn][Dd][Mm]		DO(return TOK_ENDM);

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
				  *strchr(yytext,':') = 0;
				  yylval.id = make_id(yytext);
				  return LOCAL; })}
\.[a-zA-Z_][a-zA-Z_0-9]*::	{ DO(
				  yyerror("a reusable label cannot be global");
				  )}
[a-zA-Z_][a-zA-Z_0-9]*::	{ DO({
				  *strchr(yytext,':') = 0;
				  yylval.id = make_id(yytext);
				  return GLOBAL; })}
\.[0-9][a-zA-Z_0-9]*[bB]	{ DO({
				  yytext[strlen(yytext)-1] = 0;
				  yylval.id = make_id(yytext);
				  return BACKWARDS; })}
\.[0-9][a-zA-Z_0-9]*[fF]	{ DO({
				  yytext[strlen(yytext)-1] = 0;
				  yylval.id = make_id(yytext);
				  return FORWARDS; })}
\.[0-9][a-zA-Z_0-9]*:		{ DO({
				  *strrchr(yytext,':') = 0;
				  yylval.id = make_id(yytext);
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
				  char *s;          
				  *strrchr(yytext,'"') = 0;
				  s = stralloc(yytext+1);
				  backslash(s);
				  yylval.str = s;
				  return STRING; })}

[\t ]*               		{ if (recording)
				    macro_add(yytext); }
\n				{ if (recording)
				    macro_add(yytext);
				  current_loc.line++; }
;[^\n]*\n			{ if (recording)
				    macro_add(yytext);
				  current_loc.line++; }
"//"[^\n]*\n			{ if (recording)
				    macro_add(yytext);
				  current_loc.line++; }

^#\ [0-9]+\ \"[^"]*\"(\ [0-9]+)*\n {
				  char *start = strchr(yytext,'"')+1;
				  if (recording)
				    macro_add(yytext);
				  *strchr(start,'"') = 0;
				  set_file(start);
                                  current_loc.line = strtol(yytext+2,NULL,0); }

.				DO(return *yytext);
