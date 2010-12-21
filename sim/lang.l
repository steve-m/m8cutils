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
#include <readline/readline.h>
#include <readline/history.h>

#include "util.h"
#include "cpp.h"

#include "lang.h"
#include "sim.h"
#include "id.h"
#include "printf.h"
#include "y.tab.h"


FILE *next_file = NULL;

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


static char *line,*p;


int my_yyinput(char *buffer,int max_size)
{
    int size = 0;

    if (!isatty(fileno(yyin)))
	return read(fileno(yyin),buffer,max_size);
    if (!p) {
again:
	free(line);
	line = p = readline("m8csim> ");
	if (!line)
	    return 0;
	if (!*line) {
	    HIST_ENTRY *hist = NULL;
	    int pos;

	    pos = where_history();
	    if (pos)
		hist = history_get(pos);
	    if (!hist)
		goto again;
	    p = hist->line;
	}
	else {
	    if (strspn(line," \t") != strlen(line))
		add_history(line);
	}
    }
    while (p && size != max_size) {
	if (*p)
	    *buffer++ = *p++;
	else {
	    *buffer = '\n';
	    p = NULL;
	}
	size++;
    }
    return size;
}


void my_yyrestart(void)
{
    free(line);
    line = p = NULL;
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
step				return TOK_STEP;
connect				return TOK_CONNECT;
disconnect			return TOK_DISCONNECT;
drive				return TOK_DRIVE;
set				return TOK_SET;
define				return TOK_DEFINE;
quit				return TOK_QUIT;
printf				return TOK_PRINTF;
sleep				return TOK_SLEEP;
break				return TOK_BREAK;
unbreak				return TOK_UNBREAK;

[pP][012345678]			{ yylval.num = yytext[1]-'0';
				  return PORT; }
[Rr]				return TOK_R;
[Zz]				return TOK_Z;
analog				return TOK_ANALOG;
ice				return TOK_ICE;

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
				  if (yylval.id) {
				    if (yylval.id->new)
					return NEW_ID;
				    if (yylval.id->reg != 0xffff)
					return OLD_ID;
				    yylval.num = yylval.id->field;
				    return NUMBER;
				  }
				  yylval.id = id_new(yytext);
				  return NEW_ID; }
\.[A-Za-z_][0-9A-Za-z_]*	{ yylval.id = id_new(yytext);
				  return NEW_ID; }

\"(\\[^\n\t]|[^\\"\n\t])*\"     { char *s;
				  *strrchr(yytext,'"') = 0;
				  s = stralloc(yytext+1);
				  backslash(s);
				  yylval.str = s;
				  return STRING; }

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
((;|"//")[^\n]*)?\n		{ lineno++;
				  return TOK_NL; }

^#\ [0-9]+\ \"[^"]*\"(\ [0-9]+)*\n

.				return *yytext;

%%


int interactive = 0;


void exit_if_script(int status)
{
    if (!interactive)
	exit(status);
}


void __attribute__((noreturn)) yyerrorf(const char *fmt,...)
{
    va_list ap;

    if (!isatty(fileno(yyin)))
	fprintf(stderr,"%d: ",lineno);
    va_start(ap,fmt);
    vfprintf(stderr,fmt,ap);
    fputc('\n',stderr);
    exit_if_script(1);
    longjmp(error_env,0);
}


void __attribute__((noreturn)) yyerror(const char *s)
{
    yyerrorf("%s",s);
}


int yywrap(void)
{
    if (include_default) {
	reap_cpp();
	include_default = 0;
	if (dup2(saved_stdin,0) < 0) {
	    perror("dup2");
	    exit(1);
	}
    }
    if (!next_file)
	return 1;
    yyin = next_file;
    next_file = NULL;
    interactive = isatty(fileno(yyin));
    lineno = 1;
    return 0;
}
