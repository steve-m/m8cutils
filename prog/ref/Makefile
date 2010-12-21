#
# Makefile - Makefile of the vector reference tools
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

CFLAGS=-Wall -I../../shared

.PHONY:		all clean


all:		an2026a an2026b

an2026a:	main.c an2026a-vec.inc an2026a-map.inc an2026a-data.inc \
		  ../vectors.h Makefile
		$(CC) $(CFLAGS) -o $@ main.c \
		  -DDATA_FILE='"an2026a-data.inc"' \
		  -DMAP_FILE='"an2026a-map.inc"' \
		  -DVECTOR_FILE='"an2026a-vec.inc"'

an2026b:	main.c an2026b-vec.inc an2026b-map.inc an2026b-data.inc \
		  ../vectors.h Makefile
		$(CC) $(CFLAGS) -o $@ main.c \
		  -DDATA_FILE='"an2026b-data.inc"' \
		  -DMAP_FILE='"an2026b-map.inc"' \
		  -DVECTOR_FILE='"an2026b-vec.inc"'

an2026a-map.inc: an2026a.map mapconv.pl
		perl ./mapconv.pl -m $< >$@ || { rm -f $@; exit 1; }

an2026a-data.inc: an2026a.map mapconv.pl
		perl ./mapconv.pl -d $< >$@ || { rm -f $@; exit 1; }

an2026a-vec.inc: an2026a.txt vectorconv.pl
		perl ./vectorconv.pl $< >$@ || { rm -f $@; exit 1; }

an2026b-map.inc: an2026b.map mapconv.pl
		perl ./mapconv.pl -m $< >$@ || { rm -f $@; exit 1; }

an2026b-data.inc: an2026b.map mapconv.pl
		perl ./mapconv.pl -d $< >$@ || { rm -f $@; exit 1; }

an2026b-vec.inc: an2026b.txt vectorconv.pl
		perl ./vectorconv.pl $< >$@ || { rm -f $@; exit 1; }

clean:
		rm -f an2026a-map.inc an2026a-data.inc an2026a-vec.inc
		rm -f an2026b-map.inc an2026b-data.inc an2026b-vec.inc

spotless:	clean
		rm -f an2026a an2026b
