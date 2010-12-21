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
      assert cycles \
      comment \
      arith bit shift logical relop \
      char hex decimal octal binary \
      xminus \
      evalconst \
      sym_local sym_global sym_reuse redefine \
      sep_local sep_global sep_undef sep_mix sep_export \
      sep_undef_re sep_cpp \
      dundef \
      prot_all prot_hole prot_short prot_comment \
      redef_before redef_after redef_syntax redef_undef redef_mix redef_ext \
      if if_nest if_la if_err \
      macro macro_nest macro_if macro_sep macro_err \
      include
FILES=README Makefile \
      m8cas.c m8c.l m8c.y \
      error-as.h error-as.c id.h id.c input.h op.h op.c code.h code.c \
      tests/Common $(TESTS:%=tests/%)
OBJS=lex.yy.o y.tab.o m8cas.o error-as.o id.o op.o code.o

LDLIBS += -lfl


.PHONY:		all install uninstall clean spotless


all:		m8cas

m8cas:		$(OBJS) $(LIBDEP)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

lex.yy.c:	m8c.l y.tab.h
		$(LEX) m8c.l

lex.yy.o:	lex.yy.c y.tab.h
		$(CC) -c $(CFLAGS_LEX) lex.yy.c

y.tab.c y.tab.h: m8c.y
		$(YACC) $(YYFLAGS) -d m8c.y

y.tab.o:	y.tab.c
		$(CC) -c $(CFLAGS_YACC) y.tab.c

# ----- Installation ----------------------------------------------------------

install:	all
		$(INSTALL) m8cas $(INSTALL_PREFIX)/bin/m8cas

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/bin/m8cas

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f y.tab.h y.tab.c lex.yy.c
		rm -f $(OBJS) .depend tests/_*

spotless:	clean
		rm -f m8cas
