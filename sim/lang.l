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
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <setjmp.h>

#include "util.h"
#include "lang.h"
#include "sim.h"
#include "id.h"
#include "y.tab.h"


extern FILE *next_file = NULL;

int new_line = 1;

static int lineno = 1;


#undef YY_INPUT
#define YY_INPUT(b,r,ms) (r = my_yyinput(b,ms))


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


int my_yyinput(char *buffer,int max_size)
{
    if (new_line && isatty(fileno(yyin))) {
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr,"m8csim> ");
	fflush(stderr);
	new_line = 0;
    }
    return read(fileno(yyin),buffer,max_size);
}


%}

%%

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
define				return TOK_DEFINE;

[pP][012345678]			{ yylval.num = yytext[1]-'0';
				  return PORT; }
[Rr]				return TOK_R;
[Zz]				return TOK_Z;

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

[A-Za-z_][0-9A-Za-z_]*		{ yylval.id = id_lookup(yytext);
				  if (yylval.id)
				    return yylval.id->new ? NEW_ID : OLD_ID;
				  yylval.id = id_new(yytext);
				  return NEW_ID; }

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
((;|"//")[^\n]*)?\n		{ new_line = 1;
				  lineno++;
				  return TOK_NL; }

.				return *yytext;

%%


void __attribute__((noreturn)) yyerrorf(const char *fmt,...)
{
    va_list ap;

    fprintf(stderr,"%d: ",lineno);
    va_start(ap,fmt);
    vfprintf(stderr,fmt,ap);
    fputc('\n',stderr);
    longjmp(error_env,0);
}


void __attribute__((noreturn)) yyerror(const char *s)
{
    yyerrorf("%s",s);
}


int yywrap(void)
{
    if (!next_file)
	return 1;
    yyin = next_file;
    next_file = NULL;
    lineno = 1;
    return 0;
}
