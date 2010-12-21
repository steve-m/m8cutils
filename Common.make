#
# Common.make - Common settings used by all Makefiles
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

CFLAGS_WARN=-Wall -Wstrict-prototypes -Wmissing-prototypes \
            -Wmissing-declarations -Wshadow -Werror
CFLAGS=$(CFLAGS_WARN) -g -I../shared

LDLIBS=-L../shared -lcy8c2utils
LIBDEP=../shared/libcy8c2utils.a

LEX=flex
YYFLAGS=-v
