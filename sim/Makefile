#
# Makefile - Makefile of the M8C simulator
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include ../Common.make


TESTS=adc_nc adc_c_in adc_c_out adc_nz adc_z adc_nc_z adc_c_z \
      add_c_nc add_sp and_c and_z and_nc and_nz \
      asl asr \
      call jmp lcall ljmp \
      cmp cpl dec inc \
      rlc rrc \
      swap \
      tst_ram tst_reg \
      drive_0 drive_1 drive_0r drive_1r drive_z \
      set_0 set_1 set_0r set_1r set_long_r set_z set_analog \
      intsw inttake intret \
      gpioint gpiointcfg \
      addrop printf sleep break unbreak \
      bugs7 bugs8 \
      symbols symexpr symquot regreg ppreg
FILES=README Makefile \
      lang.l lang.y lang.h id.h id.c \
      reg.h reg.c core.h core.c sim.h sim.c gpio.h gpio.c \
      int.h int.c printf.h printf.c \
      example/README example/blink.asm example/switch.asm \
      tests/Common $(TESTS:%=tests/%)
OBJS=lex.yy.o y.tab.o id.o reg.o sim.o core.o gpio.o int.o printf.o

CFLAGS += -I../prog
# don't use += because libprog may need symbols from libm8cutils, so we need to
# put -lprog before the old $(LDLIBS).
LDLIBS := -lfl -L../prog -lprog $(LDLIBS) -lreadline -lcurses
LIBDEP += ../prog/libprog.a

.PHONY:		all install uninstall clean spotless


all:		m8csim default.m8csim

m8csim:		$(OBJS) $(LIBDEP)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

lex.yy.c:	lang.l y.tab.h
		$(LEX) lang.l

lex.yy.o:	lex.yy.c y.tab.h
		$(CC) -c $(CFLAGS_LEX) lex.yy.c

y.tab.c y.tab.h: lang.y
		$(YACC) $(YYFLAGS) -d lang.y

y.tab.o:	y.tab.c
		$(CC) -c $(CFLAGS_YACC) y.tab.c

default.m8csim:	../regs/m8c-registers ../regs/capabilities ../regs/reg2hdrs.pl
		$(GEN) $(CPP) - <../regs/m8c-registers -I../regs | \
		  perl ../regs/reg2hdrs.pl sim >$@ || \
		  { rm -f $@; exit 1; }

# ----- Installation ----------------------------------------------------------

install:	all
		$(INSTALL) m8csim $(INSTALL_PREFIX)/bin/m8csim
		$(INSTALL) default.m8csim \
		  $(INSTALL_PREFIX)/share/m8cutils/include/default.m8csim

uninstall:
		$(UNINSTALL) $(INSTALL_PREFIX)/bin/m8csim
		$(UNINSTALL) \
		  $(INSTALL_PREFIX)/share/m8cutils/include/default.m8csim

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f y.tab.h y.tab.c lex.yy.c
		rm -f $(OBJS) .depend tests/_*

spotless:	clean
		rm -f m8csim default.m8csim
