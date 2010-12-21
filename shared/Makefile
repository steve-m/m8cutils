#
# Makefile - Makefile for shared parts of the M8C utilities
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=
FILES=README Makefile \
      m8c-registers reg2hdrs.pl \
      ssc.inc regdef.h \
      file.h file.c disasm.h disasm.c chips.h chips.c cpp.h cpp.c \
      interact.h interact.c symmap.h symmap.c util.h
OBJS=file.o disasm.o chips.o cpp.o interact.o symmap.o regdef.o


.PHONY:		all install uninstall clean spotless


all:		libm8cutils.a m8c.inc m8c.h ssc.h regdef.c

libm8cutils.a: $(OBJS)
		$(AR) cr$(AR_v) $@ $^

m8c.inc:	m8c-registers reg2hdrs.pl
		$(GEN) $(CPP) - <$< | perl reg2hdrs.pl asm >$@ || \
		  { rm -f $@; exit 1; }

m8c.h:		m8c-registers reg2hdrs.pl
		$(GEN) $(CPP) - <$< | perl reg2hdrs.pl h >$@ || \
		  { rm -f $@; exit 1; }

regdef.c:	m8c-registers reg2hdrs.pl
		$(GEN) $(CPP) - <$< | perl reg2hdrs.pl c >$@ || \
		  { rm -f $@; exit 1; }

ssc.h:		ssc.inc
		$(GEN) (echo '/* MACHINE-GENERATED. DO NOT EDIT ! */'; \
		  echo; sed 's/\.inc/.h/;s/_INC/_H/' <$<; ) >$@ || \
		  { rm -f $@; exit 1; }

# ----- Installation ----------------------------------------------------------

install:	all ssc.inc
		$(INSTALL) m8c.inc \
		  $(INSTALL_PREFIX)/share/m8cutils/include/m8c.inc
		$(INSTALL) ssc.inc \
		  $(INSTALL_PREFIX)/share/m8cutils/include/ssc.inc
		$(INSTALL) m8c.inc \
		  $(INSTALL_PREFIX)/share/m8cutils/include/m8c.h
		$(INSTALL) ssc.inc \
		  $(INSTALL_PREFIX)/share/m8cutils/include/ssc.h

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/share/m8cutils/include/m8c.inc
		$(UNINSTALL) $(INSTALL_PREFIX)/share/m8cutils/include/ssc.inc
		$(UNINSTALL) $(INSTALL_PREFIX)/share/m8cutils/include/m8c.h
		$(UNINSTALL) $(INSTALL_PREFIX)/share/m8cutils/include/ssc.h

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f $(OBJS) .depend

spotless:	clean
		rm -f libm8cutils.a m8c.inc m8c.h ssc.h regdef.c
