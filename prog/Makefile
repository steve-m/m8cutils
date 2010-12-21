#
# Makefile - Makefile of the M8C programmer
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


DRIVERS=waspic watsp watpp ecb_at91 dummy
TESTS=rd errd ercmok ercmf wrrd wrcmok wrcmf wrlkcmok wrlkcmf wrlkrdf wrlkrdz \
      secu secf secr secw secfsrw secfsur secfsfw \
      secfspu secfspf secfspr secfspw secmir inrom inhex outrom outhex \
      inimbin inimrom inimhex
FILES=README Makefile \
      m8cprog.c vectors.h \
      tty.h tty.c pp.h pp.c rt.h rt.c \
      prog_common.h prog_common.c prog_bit.h prog_bit.c \
      prog_vector.h prog_vector.c prog_prim.h prog_prim.c \
      prog_ops.h prog_ops.c prog.h prog.c \
      ice.h ice.c cli.h cli.c \
      torture.c \
      ref/README ref/Makefile ref/main.c \
      ref/vectorconv.pl ref/vdecode.pl ref/mapconv.pl \
      ref/an2026a.txt ref/an2026a.map ref/an2026b.txt ref/an2026b.map \
      $(DRIVERS:%=drv/%.c) drv/ecb_at91.h \
      tests/Common $(TESTS:%=tests/%)
OBJS=m8cprog.o
LIBPROG_OBJS=tty.o pp.o rt.o prog_common.o prog_bit.o prog_vector.o \
  prog_prim.o prog_ops.o prog.o ice.o cli.o $(DRIVERS:%=drv/%.o)
LIBDEP += libprog.a
# don't use += because libprog may need symbols from libm8cutils
LDLIBS := -L. -lprog $(LDLIBS)

CFLAGS += -I$(shell pwd) \
          -DPROGRAMMERS_DECL="$(DRIVERS:%=extern struct prog_common %;)" \
          -DPROGRAMMERS_OPS="$(DRIVERS:%=&%,)"


.PHONY:		all install uninstall clean spotless


all:		m8cprog

m8cprog:	$(OBJS) $(LIBDEP)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

torture:	torture.o $(LIBDEP)
		$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS) -lssl

libprog.a:	$(LIBPROG_OBJS)
		$(AR) cr$(AR_v) $@ $^

# ----- Extra dependencies ----------------------------------------------------

prog.o:		$(DRIVERS:%=drv/%.c)

DEPEND_EXTRA_CFLAGS = -Wno-error

# ----- Installation ----------------------------------------------------------

install:        all
		$(INSTALL) m8cprog $(INSTALL_PREFIX)/bin/m8cprog

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/bin/m8cprog

# ----- Cleanup ---------------------------------------------------------------

clean:
		$(MAKE) -C ref clean
		rm -f $(OBJS) $(LIBPROG_OBJS) libprog.a .depend

spotless:	clean
		$(MAKE) -C ref spotless
		rm -f m8cprog
