#
# Common.make - Common settings used by all Makefiles
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

CFLAGS_WARN=-Wall -Wstrict-prototypes -Wmissing-prototypes \
            -Wmissing-declarations -Wshadow -Werror
CFLAGS=$(CFLAGS_WARN) -g -I../shared -I../libfdr

LDLIBS=-L../shared -lm8cutils -L../libfdr -lfdr
LIBDEP=../shared/libm8cutils.a ../libfdr/libfdr.a

LEX=flex
YYFLAGS=-v
