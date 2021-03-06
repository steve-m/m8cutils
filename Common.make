#
# Common.make - Common settings used by all Makefiles (except the top-level)
#
# Written 2002-2004, 2006 by Werner Almesberger
# Copyright 2002,2003 California Institute of Technology
# Copyright 2004 Koninklijke Philips Electronics NV
# Copyright 2003, 2004, 2006 Werner Almesberger
#

include ../Config.make

CFLAGS_WARN=-Wall -Wshadow \
	    -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
SLOPPY_PROTOTYPES=-Wno-missing-prototypes -Wno-missing-declarations \
                  -Wno-strict-prototypes
SLOPPY_LOCALS=-Wno-unused
CFLAGS_COMMON=-g -I../shared -I../regs -I../libfdr \
	      -DVERSION=\"`cat ../VERSION`\" \
	      -DINSTALL_PREFIX=\"$(INSTALL_PREFIX)\"
ifeq ($(TEST32),1)
# 32 bit platforms only ! glibc required !
CFLAGS_PORTABILITY_TEST=-D__uint32_t_defined -Duint32_t='unsigned long'
endif
CFLAGS=$(CFLAGS_WARN) $(CFLAGS_COMMON) $(CFLAGS_PORTABILITY_TEST)
CFLAGS_LEX=$(CFLAGS_WARN) $(SLOPPY_PROTOTYPES) $(SLOPPY_LOCALS) \
           $(CFLAGS_COMMON)
CFLAGS_YACC=$(CFLAGS_WARN) $(SLOPPY_PROTOTYPES) $(SLOPPY_LOCALS) \
            -Wno-implicit-function-declaration $(CFLAGS_COMMON)

LDLIBS := -L../shared -lm8cutils -L../libfdr -lfdr
LIBDEP=../shared/libm8cutils.a ../libfdr/libfdr.a

# POSIX YACC doesn't have "YYEMPTY", so we have to use bison instead.
YACC=bison -y
LEX=flex
YYFLAGS=-v

INSTALL=install -D
UNINSTALL=rm -f

CPP := $(CPP)	# make sure changing CC won't affect CPP

CC_normal	:= $(CC)
LD_normal	:= $(LD)
AR_normal	:= $(AR)
YACC_normal	:= $(YACC)
LEX_normal	:= $(LEX)
INSTALL_normal	:= $(INSTALL)
UNINSTALL_normal:= $(UNINSTALL)

DEPEND_normal = \
  $(CPP) $(CFLAGS) $(DEPEND_EXTRA_CFLAGS) -MM -MG *.c >.depend || \
    { rm -f .depend; exit 1; }

CC_quiet	= @echo "  CC       " $@ && $(CC_normal)
LD_quiet	= @echo "  LD       " $@ && $(LD_normal)
AR_quiet	= @echo "  AR       " $@ && $(AR_normal)
YACC_quiet	= @echo "  YACC     " $@ && $(YACC_normal)
LEX_quiet	= @echo "  LEX      " $@ && $(LEX_normal)
GEN_quiet	= @echo "  GENERATE " $@ &&
INSTALL_quiet	= \
  @___() { echo -n "  INSTALL  " && eval echo "\$$`expr $$\#`" && \
    $(INSTALL_normal) "$$@"; }; ___
UNINSTALL_quiet	= \
  @___() { echo "  RM       " "$$1" && $(UNINSTALL_normal) "$$@"; }; ___
DEPEND_quiet	= @echo "  DEPENDENCIES" && $(DEPEND_normal)

ifeq ($(V),1)
    CC		= $(CC_normal)
    LD		= $(LD_normal)
    AR		= $(AR_normal)
    AR_v	= v
    YACC	= $(YACC_normal)
    LEX		= $(LEX_normal)
    GEN		=
    GEN_more	=
    INSTALL	= $(INSTALL_normal)
    UNINSTALL	= $(UNINSTALL_normal)
    DEPEND	= $(DEPEND_normal)
else
    CC		= $(CC_quiet)
    LD		= $(LD_quiet)
    AR		= $(AR_quiet)
    AR_v	=
    YACC	= $(YACC_quiet)
    LEX		= $(LEX_quiet)
    GEN		= $(GEN_quiet)
    GEN_more	= @
    INSTALL	= $(INSTALL_quiet)
    UNINSTALL	= $(UNINSTALL_quiet)
    DEPEND	= $(DEPEND_quiet)
endif

#
# Without this, recent makes fail, because they default to exporting
# everything. Oddly enough, variables other than CC don't seem to be affected.
#
unexport CC


# ===== Targets common to all parts ===========================================


.PHONY:	_all dep depend testlist tests filelist

_all:		all


# ----- Dependencies ----------------------------------------------------------

$(OBJS):	.depend

dep depend .depend:
		$(DEPEND)

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

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)
