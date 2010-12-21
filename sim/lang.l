%{
/*
 * lang.l - Scripting language of the simulator
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
#include "y.tab.h"


static char *file = NULL;
static int lineno = 1;


static void set_file(const char *name)
{
    free(file);
    file = stralloc(name);
}


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

%s WORD

%%

<WORD>([^\t\n ]|\\\ )+		{ yylval.str = yytext;
				  return STRING; }
		
\"([^"]|\\\")*\"		{ *strrchr(yytext,'"') = 0;
				  yylval.str = yytext+1;
				  backslash(yytext+1);
				  return STRING; }

[Aa]				return TOK_A;
[Ff]				return TOK_F;
[Ss][Pp]			return TOK_SP;
[Xx]				return TOK_X;
[Rr][Ee][Gg]			return TOK_REG;

[Rr][Oo][Mm]			return TOK_ROM;
[Rr][Aa][Mm]			return TOK_RAM;

run				return TOK_RUN;
connect				return TOK_CONNECT;
disconnect			return TOK_DISCONNECT;

[pP][012345678]			{ yylval.num = yytext[1]-'0';
				  return PORT; }
0[Xx][0-9A-Fa-f]+|[0-9]+	{ char *end;
				  yylval.num = strtoul(yytext,&end,0);
				  if (*end)
				    yyerror("invalid digit in number");
				  return NUMBER; }
[0-9A-Fa-f]+[Hh]		{ yylval.num = strtoul(yytext,NULL,16);
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

"||"				return TOK_LOGICAL_OR;
&&				return TOK_LOGICAL_AND;
"<<"				return TOK_SHL;
">>"				return TOK_SHR;
==				return TOK_EQ;
!=				return TOK_NE;
"<="				return TOK_LE;
">="				return TOK_GE;

'[^']'				{ if (!isascii(yytext[1]))
				    yyerror("non-ASCII character constant");
				  yylval.num = yytext[1];
				  return NUMBER; }
'\\''				{ yylval.num = '\'';
				  return NUMBER; }
'\\\\'				{ yylval.num = '\\';
				  return NUMBER; }

[\t ]*               
((;|"//")[^\n]*)?\n		{ BEGIN(INITIAL);
				  lineno++;
				  return TOK_NL; }

^#\ [0-9]+\ \"[^"]*\"(\ [0-9]+)*\n {
				  char *start = strchr(yytext,'"')+1;
				  *strchr(start,'"') = 0;
				  set_file(start);
                                  lineno = strtol(yytext+2,NULL,0); }

.				return *yytext;

%%


void __attribute__((noreturn)) yyerrorf(const char *fmt,...)
{
    va_list ap;

    fprintf(stderr,"%s:%d: ",file ? file : "<stdin>",lineno);
    va_start(ap,fmt);
    vfprintf(stderr,fmt,ap);
    fputc('\n',stderr);
    exit(1);
}


void __attribute__((noreturn)) yyerror(const char *s)
{
    yyerrorf("%s",s);
}
