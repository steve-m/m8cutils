%{
/*
 * lang.y - Connection checker configuration language
 *
 * Written 2006 by Werner Almesberger
 * Copyright 2006 Werner Almesberger
 */


#include <stdint.h>
#include <string.h>

#include "error.h"

#include "y.tab.h"

%}

%%

[Aa][Ll][Ll][Oo][Ww]			return ALLOW;
[Ee][Xx][Tt][Ee][Rr][Nn][Aa][Ll]	return EXTERNAL;

[0-7]		{ yylval.num = yytext[0]-'0';
		  return BIT; }

[Pp][0-7]	{ yylval.num = yytext[1] -'0';
		  return PORT; }

[Rr]		return R;
[Zz]		return Z;

[\t ]*
\n		current_loc.line++;
;[^\n]*\n	current_loc.line++;
"//"[^\n]*\n	current_loc.line++;

^#\ [0-9]+\ \"[^"]*\"(\ [0-9]+)*\n {
		  char *start = strchr(yytext,'"')+1;
		  *strchr(start,'"') = 0;
		  set_file(start);
		  current_loc.line = strtol(yytext+2,NULL,0); }

.		return *yytext;
