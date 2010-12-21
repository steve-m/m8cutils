#
# Makefile - Makefile for shared parts of the M8C utilities
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


FILES=README Makefile \
      m8c-registers reg2hdrs.pl \
      ssc.inc \
      file.h file.c disasm.h disasm.c chips.h chips.c cpp.h cpp.c \
      interact.h interact.c symmap.h symmap.c
OBJS=file.o disasm.o chips.o cpp.o interact.o symmap.o


.PHONY:		all dep depend testlist tests install uninstall filelist
.PHONY:		clean spotless


all:		libm8cutils.a m8c.inc m8c.h ssc.h

libm8cutils.a: $(OBJS)
		$(AR) cr$(AR_v) $@ $^

m8c.inc:	m8c-registers reg2hdrs.pl
		$(GEN) $(CPP) - <$< | perl reg2hdrs.pl asm >$@ || \
		  { rm -f $@; exit 1; }

m8c.h:		m8c-registers reg2hdrs.pl
		$(GEN) $(CPP) - <$< | perl reg2hdrs.pl c >$@ || \
		  { rm -f $@; exit 1; }

ssc.h:		ssc.inc
		$(GEN) (echo '/* MACHINE-GENERATED. DO NOT EDIT ! */'; \
		  echo; sed 's/\.inc/.h/;s/_INC/_H/' <$<; ) >$@ || \
		  { rm -f $@; exit 1; }

# ----- Dependencies ----------------------------------------------------------

$(OBJS):	.depend

dep depend .depend:
		$(CPP) $(CFLAGS) -MM -MG *.c >.depend || \
		  { rm -f .depend; exit 1; }

ifeq (.depend,$(wildcard .depend))
include .depend
endif

# ----- Tests -----------------------------------------------------------------

testlist:

tests:

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

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f $(OBJS) .depend

spotless:	clean
		rm -f libm8cutils.a m8c.inc m8c.h ssc.h
