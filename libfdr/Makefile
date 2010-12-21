#
# libfdr/Makefile - Makefile for red-black trees
#
# Written 2002-2004, 2006 by Werner Almesberger
# Copyright 2002, 2003 California Institute of Technology
# Copyright 2004 Koninklijke Philips Electronics NV
# Copyright 2003, 2006 Werner Almesberger
#

include ../Common.make

FILES=README LICENSE Makefile \
  jrb.c jrb.h jval.c jval.h
OBJS=jrb.o jval.o

CFLAGS=-g -O $(CFLAGS_WARN)

.PHONY:		all dep depend testlist tests install uninstall filelist
.PHONY:		clean spotless


all:		libfdr.a

libfdr.a:	$(OBJS)
		$(AR) cr$(AR_v) $@ $^

# ----- Dependencies ----------------------------------------------------------

$(OBJS):	.depend

dep depend .depend:
		$(DEPEND)

ifeq (.depend,$(wildcard .depend))
include .depend
endif

# ----- Tests -----------------------------------------------------------------

testlist:

tests:

# ----- Installation ----------------------------------------------------------

install:	all

uninstall:

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f $(OBJS) .depend

spotless:	clean
		rm -f libfdr.a
