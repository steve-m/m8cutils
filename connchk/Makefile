#
# Makefile - Makefile of the connection checker
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=
FILES=README Makefile \
      connchk.c lang.l lang.y port.h port.c value.h value.c out.h out.c \
      example/README example/bare.env example/short.env example/short2.env \
      example/resistor.env example/diode.env \
      test.h test.c \
      tests/Common $(TESTS:%=tests/%)
OBJS=lex.yy.o y.tab.o connchk.o port.o value.o out.o test.o

CFLAGS += -I../prog
# don't use += because libprog may need symbols from libm8cutils, so we
# need to put -lprog before the old $(LDLIBS).
LDLIBS := -lfl -L../prog -lprog $(LDLIBS)
LIBDEP += ../prog/libprog.a


.PHONY:		all install uninstall clean spotless


all:		connchk

connchk:	$(OBJS) $(LIBDEP)
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
#		$(INSTALL) connchk $(INSTALL_PREFIX)/bin/connchk

uninstall:
#		$(UNINSTALL) $(INSTALL_PREFIX)/bin/connchk

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f y.tab.h y.tab.c lex.yy.c
		rm -f $(OBJS) .depend tests/_*

spotless:	clean
		rm -f connchk connchk.exe
