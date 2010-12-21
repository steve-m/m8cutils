#
# Makefile - Makefile of the cy8c2prog programmer
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


FILES=README Makefile \
      cy8c2prog.h cy8c2prog.c cy8c2regs.h vectors.h chips.h chips.c \
      tty.h tty.c prog.h prog.c \
      prog_waspic.c
OBJS=cy8c2prog.o chips.o tty.o prog.o prog_waspic.o


.PHONY:		all dep depend tests filelist clean spotless


all:		cy8c2prog

cy8c2prog:	$(OBJS) $(LIBDEP)

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
		rm -f cy8c2prog .depend
