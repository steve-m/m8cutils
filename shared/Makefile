#
# Makefile - Makefile for shared parts of the CY8C2 utilities
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


FILES=README Makefile \
      file.h file.c disasm.h disasm.c
OBJS=file.o disasm.o


.PHONY:		all dep depend tests filelist clean spotless


all:		libcy8c2utils.a

libcy8c2utils.a: $(OBJS)
		$(AR) crv $@ $^


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
		rm -f libcy8c2utils.a .depend
