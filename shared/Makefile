#
# Makefile - Makefile for shared parts of the M8C utilities
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=
FILES=README Makefile \
      ssc.inc \
      file.h file.c disasm.h disasm.c chips.h chips.c cpp.h cpp.c \
      interact.h interact.c symmap.h symmap.c util.h error.h error.c \
      backslash.h backslash.c security.h security.c
OBJS=file.o disasm.o chips.o cpp.o interact.o symmap.o error.o backslash.o \
     security.o
CFLAGS += -DCPP='"$(CPP_COMMAND)"'


.PHONY:		all install uninstall clean spotless


all:		libm8cutils.a ssc.h

libm8cutils.a: $(OBJS)
		$(AR) cr$(AR_v) $@ $^

ssc.h:		ssc.inc
		$(GEN) (echo '/* MACHINE-GENERATED. DO NOT EDIT ! */'; \
		  echo; sed 's/\.inc/.h/;s/_INC/_H/' <$<; ) >$@ || \
		  { rm -f $@; exit 1; }

# ----- Installation ----------------------------------------------------------

install:	ssc.h ssc.inc
		$(INSTALL) ssc.inc \
		  $(INSTALL_PREFIX)/share/m8cutils/include/ssc.inc
		$(INSTALL) ssc.h \
		  $(INSTALL_PREFIX)/share/m8cutils/include/ssc.h

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/share/m8cutils/include/ssc.inc
		$(UNINSTALL) $(INSTALL_PREFIX)/share/m8cutils/include/ssc.h

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f $(OBJS) .depend

spotless:	clean
		rm -f libm8cutils.a ssc.h
