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
      exceed output input pc
FILES=README Makefile \
      m8cdasm.c \
      tests/Common $(TESTS:%=tests/%)
OBJS=m8cdasm.o


.PHONY:		all dep depend tests filelist clean spotless


all:		m8cdasm

m8cdasm:	$(OBJS)	$(LIBDEP)

# ----- Dependencies ----------------------------------------------------------

$(OBJS):	.depend

dep depend .depend:
		$(CPP) $(CFLAGS) -MM -MG *.c >.depend || \
		  { rm -f .depend; exit 1; }

ifeq (.depend,$(wildcard .depend))
include .depend               
endif

# ----- Tests -----------------------------------------------------------------

tests:		all
		passed=0 && cd tests && for n in $(TESTS); do \
		  SCRIPT=$$n . ./$$n; done; \
		  echo "Passed all $$passed tests" 2>&1

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f y.tab.h y.tab.c lex.yy.c
		rm -f $(OBJS) tests/_*

spotless:	clean
		rm -f m8cdasm .depend
