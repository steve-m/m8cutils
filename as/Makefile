#
# Makefile - Makefile of the M8C assembler
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=reas_0 reas_1 reas_2 reas_3 reas_4 reas_5 reas_6 reas_7 \
      reas_8 reas_9 reas_abcd reas_e reas_f \
      byte_bwd byte_fwd word_bwd word_fwd off1_bwd off1_fwd off2_bwd off2_fwd \
      mvi \
      reuse_bf reuse_fb reuse_undef reuse_before reuse_after \
      reuse_global reuse_export \
      area asciz blk blkw db ds dw dwl equ export org \
      comment \
      arith bit shift logical relop \
      char hex decimal octal binary \
      xminus \
      evalconst \
      sym_local sym_global sym_reuse redefine
FILES=README Makefile \
      m8cas.c m8c.l m8c.y \
      util.h error.h error.c id.h id.c op.h op.c code.h code.c \
      tests/Common $(TESTS:%=tests/%)
OBJS=lex.yy.o y.tab.o m8cas.o error.o id.o op.o code.o

LDLIBS += -lfl


.PHONY:		all dep depend testlist tests install uninstall filelist
.PHONY:		clean spotless


all:		m8cas

m8cas:		$(OBJS) $(LIBDEPS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

lex.yy.c:	m8c.l y.tab.h
		$(LEX) m8c.l

lex.yy.o:	lex.yy.c y.tab.h
		$(CC) -c $(CFLAGS_LEX) lex.yy.c

y.tab.c y.tab.h: m8c.y
		$(YACC) $(YYFLAGS) -d m8c.y

y.tab.o:	y.tab.c
		$(CC) -c $(CFLAGS_YACC) y.tab.c

# ----- Dependencies ----------------------------------------------------------

dep depend .depend:
		$(CPP) $(CFLAGS) -MM -MG *.c >.depend || \
		  { rm -f .depend; exit 1; }

ifeq (.depend,$(wildcard .depend))
include .depend               
endif

# ----- Tests -----------------------------------------------------------------

testlist:
		@echo $(TESTS)

tests:		all
		passed=0 && cd tests && for n in $(TESTS); do \
		  SCRIPT=$$n . ./$$n; done; \
		  echo "Passed all $$passed tests" 2>&1

# ----- Installation ----------------------------------------------------------

install:	all
		$(INSTALL) m8cas $(INSTALL_PREFIX)/bin/m8cas

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/bin/m8cas

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f y.tab.h y.tab.c lex.yy.c
		rm -f $(OBJS) .depend tests/_*

spotless:	clean
		rm -f m8cas
