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
      asciz blk blkw db ds dw dwl equ export org \
      comment
FILES=README Makefile \
      m8cas.c m8c.l m8c.y \
      util.h error.h error.c id.h id.c op.h op.c code.h code.c \
      cpp.h cpp.c \
      tests/Common $(TESTS:%=tests/%)
OBJS=lex.yy.o y.tab.o m8cas.o error.o id.o op.o code.o cpp.o

LDLIBS += -lfl


.PHONY:		all dep depend filelist clean spotless


all:		m8cas

m8cas:		$(OBJS)
		$(CC) -o $@ $^ $(LDLIBS)

lex.yy.c:	m8c.l y.tab.h
		$(LEX) m8c.l

lex.yy.o:	lex.yy.c y.tab.h
		$(CC) -c $(CFLAGS) -w lex.yy.c

y.tab.c y.tab.h: m8c.y
		@echo "expect 4 shift/reduce conflicts" 1>&2
		$(YACC) $(YYFLAGS) -d m8c.y

y.tab.o:	y.tab.c
		$(CC) -c $(CFLAGS) -w y.tab.c

# ----- Dependencies ----------------------------------------------------------

dep depend .depend:
		$(CPP) $(CFLAGS) -MM -MG *.c >.depend || \
		  { rm -f .depend; exit 1; }

ifeq (.depend,$(wildcard .depend))
include .depend               
endif

# ----- Tests -----------------------------------------------------------------

tests:		all
		passed=0 && cd tests && for n in $(TESTS); do \
		  SCRIPT=$$n . ./$$n; done; \
		  echo "Passed all $$passed tests" 2>&1

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f y.tab.h y.tab.c lex.yy.c
		rm -f $(OBJS)

spotless:	clean
		rm -f m8cas .depend
