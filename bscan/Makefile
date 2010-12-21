#
# Makefile - Makefile of the connection checker
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=dm
FILES=README Makefile \
      bscan.c lang.l lang.y port.h port.c value.h value.c out.h out.c \
      test.h test.c \
      models.m8cbscan \
      example/README example/bare.env example/short.env example/short2.env \
      example/resistor.env example/diode.env \
      ident/README ident/all ident/fmt.pl ident/env \
      tests/Common $(TESTS:%=tests/%)
OBJS=lex.yy.o y.tab.o bscan.o port.o value.o out.o test.o

CFLAGS += -I../prog
# don't use += because libprog may need symbols from libm8cutils, so we
# need to put -lprog before the old $(LDLIBS).
LDLIBS := -lfl -L../prog -lprog $(LDLIBS)
LIBDEP += ../prog/libprog.a


.PHONY:		all install uninstall clean spotless


all:		m8cbscan

m8cbscan:	$(OBJS) $(LIBDEP)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

lex.yy.c:	lang.l y.tab.h
		$(LEX) lang.l

lex.yy.o:	lex.yy.c y.tab.h
		$(CC) -c $(CFLAGS_LEX) lex.yy.c

y.tab.c y.tab.h: lang.y
		$(YACC) $(YYFLAGS) -d lang.y

y.tab.o:	y.tab.c
		$(CC) -c $(CFLAGS_YACC) y.tab.c

# ----- Installation ----------------------------------------------------------

install:	all
		$(INSTALL) m8cbscan $(INSTALL_PREFIX)/bin/m8cbscan

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/bin/m8cbscan

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f y.tab.h y.tab.c lex.yy.c
		rm -f $(OBJS) .depend tests/_*

spotless:	clean
		rm -f m8cbscan m8cbscan.exe
