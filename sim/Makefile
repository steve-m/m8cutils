#
# Makefile - Makefile of the M8C simulator
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=adc_nc adc_c_in adc_c_out adc_nz adc_z adc_nc_z adc_c_z \
      add_c_nc add_sp and_c and_z and_nc and_nz \
      call jmp \
      drive_0 drive_1 drive_0r drive_1r drive_z \
      intsw inttake intret \
      gpioint \
      bugs7
FILES=README Makefile \
      lang.l lang.y lang.h id.h id.c \
      util.h reg.h reg.c core.h core.c sim.h sim.c gpio.h gpio.c \
      int.h int.c \
      example/README example/blink.asm \
      tests/Common $(TESTS:%=tests/%)
OBJS=lex.yy.o y.tab.o id.o reg.o sim.o core.o gpio.o int.o

CFLAGS += -I../prog
# don't use += because libprog may need symbols from libm8cutils
LDLIBS := -lfl -L../prog -lprog $(LDLIBS) -lreadline


.PHONY:		all dep depend testlist tests install uninstall filelist
.PHONY:		clean spotless


all:		m8csim default.m8csim

m8csim:		$(OBJS) $(LIBDEP)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

lex.yy.c:	lang.l y.tab.h
		$(LEX) lang.l

lex.yy.o:	lex.yy.c y.tab.h
		$(CC) -c $(CFLAGS_LEX) lex.yy.c

y.tab.c y.tab.h: lang.y
		$(YACC) $(YYFLAGS) -d lang.y

y.tab.o:	y.tab.c registers.h
		$(CC) -c $(CFLAGS_YACC) y.tab.c

core.o gpio.o int.o y.tab.o: \
		registers.h

registers.h:	../shared/m8c-registers ../shared/reg2hdrs.pl
		perl ../shared/reg2hdrs.pl c >$@ $< || { rm -rf $@; exit 1; }

default.m8csim:	../shared/m8c-registers ../shared/reg2hdrs.pl
		perl ../shared/reg2hdrs.pl sim >$@ $< || { rm -rf $@; exit 1; }

# ----- Dependencies ----------------------------------------------------------

dep depend .depend:
		$(CPP) $(CFLAGS) -MM -MG *.c >.depend || \
		  { rm -f .depend; exit 1; }

ifeq (.depend,$(wildcard .depend))
include .depend               
endif

# ----- Tests -----------------------------------------------------------------

testlist:
		@echo $(TESTS)

tests:		all
		passed=0 && cd tests && for n in $(TESTS); do \
		  SCRIPT=$$n . ./$$n; done; \
		  echo "Passed all $$passed tests" 2>&1

# ----- Installation ----------------------------------------------------------

install:	all
		$(INSTALL) m8csim $(INSTALL_PREFIX)/bin/m8csim
		$(INSTALL) default.m8csim \
		  $(INSTALL_PREFIX)/share/m8cutils/include/default.m8csim

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/bin/m8csim
		$(UNINSTALL) \
		  $(INSTALL_PREFIX)/share/m8cutils/include/default.m8csim

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f y.tab.h y.tab.c lex.yy.c registers.h
		rm -f $(OBJS) .depend tests/_*

spotless:	clean
		rm -f m8csim default.m8csim