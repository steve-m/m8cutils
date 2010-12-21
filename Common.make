#
# Common.make - Common settings used by all Makefiles
#
# Written 2002-2004, 2006 by Werner Almesberger
# Copyright 2002,2003 California Institute of Technology
# Copyright 2004 Koninklijke Philips Electronics NV
# Copyright 2003, 2004, 2006 Werner Almesberger
#

CFLAGS_WARN=-Wall -Wstrict-prototypes -Wmissing-prototypes \
            -Wmissing-declarations -Wshadow -Werror
CFLAGS=$(CFLAGS_WARN) -g -I../shared -I../libfdr

LDLIBS := -L../shared -lm8cutils -L../libfdr -lfdr
LIBDEP=../shared/libm8cutils.a ../libfdr/libfdr.a

LEX=flex
YYFLAGS=-v

CC_normal   := $(CC)
LD_normal   := $(LD)
AR_normal   := $(AR)
YACC_normal := $(YACC)
LEX_normal  := $(LEX)

CC_quiet    = @echo "  CC       " $@ && $(CC_normal)
LD_quiet    = @echo "  LD       " $@ && $(LD_normal)
AR_quiet    = @echo "  AR       " $@ && $(AR_normal)
YACC_quiet  = @echo "  YACC     " $@ && $(YACC_normal)
LEX_quiet   = @echo "  LEX      " $@ && $(LEX_normal)
GEN_quiet   = @echo "  GENERATE " $@ &&

ifdef V
    CC   = $(CC_normal)
    LD   = $(LD_normal)
    AR   = $(AR_normal)
    AR_v = v
    YACC = $(YACC_normal)
    LEX  = $(LEX_normal)
    GEN  =
    GEN_more =
else
    CC   = $(CC_quiet)
    LD   = $(LD_quiet)
    AR   = $(AR_quiet)
    AR_v =
    YACC = $(YACC_quiet)
    LEX  = $(LEX_quiet)
    GEN  = $(GEN_quiet)
    GEN_more = @
endif

#
# Without this, recent makes fail, because they default to exporting
# everything. Oddly enough, variables other than CC don't seem to be affected.
#
unexport CC
