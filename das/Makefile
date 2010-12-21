#
# Makefile - Makefile of the M8C disassembler
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=adc add and asl asr call cmp cpl dec halt inc index \
      jacc jc jmp jnc jnz jz lcall ljmp mov mvi nop or pop push \
      ret reti rlc romx rrc sub sbb swap ssc tst xor \
      exceed output input pc \
      sym_rom sym_ram sym_redef
FILES=README Makefile \
      m8cdas.c \
      tests/Common $(TESTS:%=tests/%)
OBJS=m8cdas.o


.PHONY:		all install uninstall clean spotless


all:		m8cdas

m8cdas:		$(OBJS)	$(LIBDEP)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# ----- Installation ----------------------------------------------------------

install:	all
		$(INSTALL) m8cdas $(INSTALL_PREFIX)/bin/m8cdas

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/bin/m8cdas

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f y.tab.h y.tab.c lex.yy.c
		rm -f $(OBJS) .depend tests/_*

spotless:	clean
		rm -f m8cdas m8cdas.exe
