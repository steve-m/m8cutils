#
# Makefile - Makefile for M8C register definition processing
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=
FILES=README Makefile \
      m8c-registers capabilities reg2hdrs.pl \
      regdef.h reglib.c
OBJS=regdef.o reglib.o


.PHONY:		all install uninstall clean spotless


all:		m8c.inc m8c.h libregdef.a

libregdef.a:	$(OBJS)
		$(AR) cr$(AR_v) $@ $^

m8c.inc:	m8c-registers capabilities reg2hdrs.pl
		$(GEN) $(CPP) - <$< | perl reg2hdrs.pl asm >$@ || \
		  { rm -f $@; exit 1; }

m8c.h:		m8c-registers capabilities reg2hdrs.pl
		$(GEN) $(CPP) - <$< | perl reg2hdrs.pl h >$@ || \
		  { rm -f $@; exit 1; }

regdef.c:	m8c-registers capabilities reg2hdrs.pl
		$(GEN) $(CPP) - <$< | perl reg2hdrs.pl c >$@ || \
		  { rm -f $@; exit 1; }

# ----- Installation ----------------------------------------------------------

install:	m8c.inc m8c.h
		$(INSTALL) m8c.inc \
		  $(INSTALL_PREFIX)/share/m8cutils/include/m8c.inc
		$(INSTALL) m8c.h \
		  $(INSTALL_PREFIX)/share/m8cutils/include/m8c.h

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/share/m8cutils/include/m8c.inc
		$(UNINSTALL) $(INSTALL_PREFIX)/share/m8cutils/include/m8c.h

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f $(OBJS) .depend

spotless:	clean
		rm -f m8c.inc m8c.h regdef.c libregdef.a
