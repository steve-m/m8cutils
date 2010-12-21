#
# Makefile - Makefile for shared parts of the M8C utilities
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


FILES=README Makefile \
      file.h file.c disasm.h disasm.c chips.h chips.c cpp.h cpp.c \
      interact.h interact.c
OBJS=file.o disasm.o chips.o cpp.o interact.o


.PHONY:		all dep depend tests filelist clean spotless


all:		libm8cutils.a

libm8cutils.a: $(OBJS)
		$(AR) cr$(AR_v) $@ $^

# ----- Dependencies ----------------------------------------------------------

$(OBJS):	.depend

dep depend .depend:
		$(CPP) $(CFLAGS) -MM -MG *.c >.depend || \
		  { rm -f .depend; exit 1; }

ifeq (.depend,$(wildcard .depend))
include .depend
endif

# ----- Tests -----------------------------------------------------------------

tests:

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f $(OBJS)

spotless:	clean
		rm -f libm8cutils.a .depend
