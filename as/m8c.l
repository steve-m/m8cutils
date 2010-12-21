%{
/*
 * m8c.y - M8C assembler language
 * 
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */

/*
 * "backslash" is from umlsim's script.l (http://umlsim.sourceforge.net/),
 * which is written 2002-2004 by Werner Almesberger
 * Copyright 2002,2003 California Institute of Technology
 * Copyright 2004 Koninklijke Philips Electronics NV
 * Copyright 2003,2004 Werner Almesberger
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

#include "y.tab.h"


int lineno = 1;


static void backslash(char *s)
{
    char *t;

    for (t = s; *s; s++) {
	if (*s != '\\') {
	    *t++ = *s;
	    continue;
	}
	switch (*++s) {
	    case 'n':
		*t++ = 10;
		break;
	    case 'r':
		*t++ = 13;
		break;
	    case 't':
		*t++ = 9;
		break;
	    case 'b':
		*t++ = 8;
		break;
	    case '\'':
		*t++ = '\'';
		break;
	    case '"':
		*t++ = '"';
		break;
	    case '\\':
		*t++ = '\\';
		break;
	    default:
		if (*s >= '0' && *s <= '7') {
		    int v = 0,n;

		    for (n = 3; n; n--) {
			if (*s < '0' || *s > '7')
			    break;
			v = v*8+*s++-'0';
		    }
		    s--;
		    if (v <= 255) {
			*t++ = v;
			break;
		    }
		}
		yyerrorf(*s > ' ' && *s <= '~' ?
		  "unrecognized escape sequence \\%c" :
		  "unrecognized escape sequence \\%03o",*s);
	}
    }
    *t = 0;
}


%}

%%

[Aa]				return TOK_A;
[Ff]				return TOK_F;
[Ss][Pp]			return TOK_SP;
[Xx]				return TOK_X;
[Rr][Ee][Gg]			return TOK_REG;

[Aa][Dd][Cc]			return TOK_ADC;
[Aa][Dd][Dd]			return TOK_ADD;
[Aa][Nn][Dd]			return TOK_AND;
[Aa][Ss][Ll]			return TOK_ASL;
[Aa][Ss][Rr]			return TOK_ASR;
[Cc][Aa][Ll][Ll]		return TOK_CALL;
[Cc][Mm][Pp]			return TOK_CMP;
[Cc][Pp][Ll]			return TOK_CPL;
[Dd][Ee][Cc]			return TOK_DEC;
[Hh][Aa][Ll][Tt]		return TOK_HALT;
[Ii][Nn][Cc]			return TOK_INC;
[Ii][Nn][Dd][Ee][Xx]		return TOK_INDEX;
[Jj][Aa][Cc][Cc]		return TOK_JACC;
[Jj][Cc]			return TOK_JC;
[Jj][Mm][Pp]			return TOK_JMP;
[Jj][Nn][Cc]			return TOK_JNC;
[Jj][Nn][Zz]			return TOK_JNZ;
[Jj][Zz]			return TOK_JZ;
[Ll][Cc][Aa][Ll][Ll]		return TOK_LCALL;
[Ll][Jj][Mm][Pp]		return TOK_LJMP;
[Mm][Oo][Vv]			return TOK_MOV;
[Mm][Vv][Ii]			return TOK_MVI;
[Nn][Oo][Pp]			return TOK_NOP;
[Oo][Rr]			return TOK_OR;
[Pp][Oo][Pp]			return TOK_POP;
[Pp][Uu][Ss][Hh]		return TOK_PUSH;
[Rr][Ee][Tt]			return TOK_RET;
[Rr][Ee][Tt][Ii]		return TOK_RETI;
[Rr][Ll][Cc]			return TOK_RLC;
[Rr][Oo][Mm][Xx]		return TOK_ROMX;
[Rr][Rr][Cc]			return TOK_RRC;
[Ss][Bb][Bb]			return TOK_SBB;
[Ss][Uu][Bb]			return TOK_SUB;
[Ss][Ww][Aa][Pp]		return TOK_SWAP;
[Ss][Ss][Cc]			return TOK_SSC;
[Tt][Ss][Tt]			return TOK_TST;
[Xx][Oo][Rr]			return TOK_XOR;

[Aa][Rr][Ee][Aa]		return TOK_AREA;
[Aa][Ss][Cc][Ii][Zz]		return TOK_ASCIZ;
[Bb][Ll][Kk]			return TOK_BLK;
[Bb][Ll][Kk][Ww]		return TOK_BLKW;
[Dd][Bb]			return TOK_DB;
[Dd][Ss]			return TOK_DS;
[Dd][Ss][Uu]			return TOK_DSU;
[Dd][Ww]			return TOK_DW;
[Dd][Ww][Ll]			return TOK_DWL;
[Ee][Ll][Ss][Ee]		return TOK_ELSE;
[Ee][Nn][Dd][Ii][Ff]		return TOK_ENDIF;
[Ee][Nn][Dd][Mm]		return TOK_ENDM;
[Ee][Qq][Uu]			return TOK_EQU;
[Ee][Xx][Pp][Oo][Rr][Tt]	return TOK_EXPORT;
[Ii][Ff]			return TOK_IF;
[Ii][Nn][Cc][Ll][Uu][Dd][Ee]	return TOK_INCLUDE;
\.[Ll][Ii][Tt][Ee][Rr][Aa][Ll]	return TOK_LITERAL;
\.[Ee][Nn][Dd][Ll][Ii][Tt][Ee][Rr][Aa][Ll] return TOK_ENDLITERAL;
[Mm][Aa][Cc][Rr][Oo]		return TOK_MACRO;
[Oo][Rr][Gg]			return TOK_ORG;
\.[Ss][Ee][Cc][Tt][Ii][Oo][Nn]	return TOK_SECTION;
\.[Ee][Nn][Dd][Ss][Ee][Cc][Tt][Ii][Oo][Nn] return TOK_ENDSECTION;

\.?[a-zA-Z_][a-zA-Z_0-9]*	{ yylval.id = make_id(yytext);
				  return LABEL; }
\.?[a-zA-Z_][a-zA-Z_0-9]*:	{ *strchr(yytext,':') = 0;
				  yylval.id = make_id(yytext);
				  return LOCAL; }
[a-zA-Z_][a-zA-Z_0-9]*::	{ *strchr(yytext,':') = 0;
				  yylval.id = make_id(yytext);
				  return GLOBAL; }

"||"				return TOK_LOGICAL_OR;
&&				return TOK_LOGICAL_AND;
"<<"				return TOK_SHL;
">>"				return TOK_SHR;
==				return TOK_EQ;
!=				return TOK_NE;
"<="				return TOK_LE;
">="				return TOK_GE;
"++"				return TOK_DOUBLE_PLUS;

'[^']'				{ if (!isascii(yytext[1]))
				    yyerror("non-ASCII character constant");
				  yylval.num = yytext[1];
				  return NUMBER; }
'\\''				{ yylval.num = '\'';
				  return NUMBER; }
'\\\\'				{ yylval.num = '\\';
				  return NUMBER; }
\"(\\[^\n\t]|[^\\"\n\t])*\"	{ char *s;          
				  *strrchr(yytext,'"') = 0;
				  s = stralloc(yytext+1);
				  backslash(s);
				  yylval.str = s;
				  return STRING; }


0[Xx][0-9A-Fa-f]+|[0-9]+	{ char *end;
				  yylval.num = strtoul(yytext,&end,0);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; }
[0-9A-Fa-f]+[Hh]		{ char *end;
				  yylval.num = strtoul(yytext+1,&end,16);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; }
$[0-9A-Fa-f]+			{ char *end;
				  yylval.num = strtoul(yytext+1,&end,16);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; }
0b[0-1]+			{ char *end;
				  yylval.num = strtoul(yytext+2,&end,2);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; }
%[0-1]+				{ char *end;
				  yylval.num = strtoul(yytext+1,&end,2);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; }

[\t ]*               
\n				current_loc.line++;
;[^\n]*\n			current_loc.line++;
"//"[^\n]*\n			current_loc.line++;

^#\ [0-9]+\ \"[^"]*\"(\ [0-9]+)*\n {
				  char *start = strchr(yytext,'"')+1;
				  *strchr(start,'"') = 0;
				  set_file(start);
                                  current_loc.line = strtol(yytext+2,NULL,0); }

.				return *yytext;

%%


void __attribute__((noreturn)) yyerrorf(const char *fmt,...)
{
    va_list ap;

    vlerrorf(&current_loc,fmt,ap);
    exit(1);
}


void __attribute__((noreturn)) yyerror(const char *s)
{
    lerrorf(&current_loc,"%s",s);
}
