/*
 * backslash.c - Backslash processing for C strings
 *
 * From umlsim's script.l (http://umlsim.sourceforge.net/),
 * which is written 2002-2004 by Werner Almesberger
 * Copyright 2002,2003 California Institute of Technology
 * Copyright 2004 Koninklijke Philips Electronics NV
 * Copyright 2003,2004 Werner Almesberger
 */


#include "error.h"
#include "backslash.h"


void backslash(char *s)
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
