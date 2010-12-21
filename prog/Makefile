#
# Makefile - Makefile of the cy8c2prog programmer
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

FILES=README TODO Makefile \
      cy8c2prog.h cy8c2prog.c cy8c2regs.h vectors.h chips.h chips.c \
      tty.h tty.c prog.h prog.c file.h file.c \
      prog_waspic.c
OBJS=cy8c2prog.o chips.o tty.o prog.o file.o prog_waspic.o

CFLAGS=-Wall -g
LDFLAGS=


.PHONY:		dep filelist clean spotless

cy8c2prog:	$(OBJS)


# ----- Dependencies ----------------------------------------------------------


$(OBJS):	.depend

dep .depend:
		$(CPP) $(CFLAGS) -MM -MG *.c >.depend || \
		  { rm -f .depend; exit 1; }

ifeq (.depend,$(wildcard .depend))
include .depend
endif


# ----- Distribution ----------------------------------------------------------


filelist:
		echo $(FILES)


# ----- Cleanup ---------------------------------------------------------------


clean:
		rm -f $(OBJS)

spotless:	clean
		rm -f cy8c2prog
