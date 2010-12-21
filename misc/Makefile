#
# Makefile - Makefile of the miscellaneous programs
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=
FILES=README Makefile \
      reginfo.c
#      tests/Common $(TESTS:%=tests/%)
OBJS=reginfo.o
LDLIBS += -L../regs -lregdef


.PHONY:		all install uninstall clean spotless


all:		reginfo

reginfo:	$(OBJS)	$(LIBDEP)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# ----- Installation ----------------------------------------------------------

install:	all
#		$(INSTALL) m8cdas $(INSTALL_PREFIX)/bin/m8cdas

uninstall:
#		$(UNINSTALL) $(INSTALL_PREFIX)/bin/m8cdas

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f $(OBJS) .depend tests/_*

spotless:	clean
		rm -f reginfo
