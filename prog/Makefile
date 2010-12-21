#
# Makefile - Makefile of the M8C programmer
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


DRIVERS=waspic watsp
FILES=README Makefile \
      m8cprog.c cy8c2regs.h vectors.h \
      tty.h tty.c prog.h prog.c ops.h ops.c ice.h ice.c \
      ref/Makefile ref/main.c \
      ref/vectorconv.pl ref/vdecode.pl ref/mapconv.pl \
      ref/an2026a.txt ref/an2026a.map ref/an2026b.txt ref/an2026b.map \
      $(DRIVERS:%=drv/%.c)
OBJS=m8cprog.o
LIBPROG_OBJS=tty.o prog.o ops.o ice.o $(DRIVERS:%=drv/%.o)
LIBDEP += libprog.a
# don't use += because libprog may need symbols from libm8cutils
LDLIBS := -L. -lprog $(LDLIBS)

CFLAGS += -I$(shell pwd) \
          -DPROGRAMMERS_DECL="$(DRIVERS:%=extern struct prog_ops %_ops;)" \
          -DPROGRAMMERS_OPS="$(DRIVERS:%=&%_ops,)"


.PHONY:		all dep depend testlist tests install uninstall filelist
.PHONY:		clean spotless


all:		m8cprog

m8cprog:	$(OBJS) $(LIBDEP)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

libprog.a:	$(LIBPROG_OBJS)
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

testlist:

tests:

# ----- Installation ----------------------------------------------------------

install:        all
		$(INSTALL) m8cprog $(INSTALL_PREFIX)/bin/m8cprog

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/bin/m8cprog

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		$(MAKE) -C ref clean
		rm -f $(OBJS) $(LIBPROG_OBJS) .depend

spotless:	clean
		$(MAKE) -C ref spotless
		rm -f m8cprog
