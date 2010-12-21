#
# Makefile - Makefile of Werner's slow PIC-based programmer
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

# ----- User-configurable items -----------------------------------------------

PORT = $(shell echo $${PICSTART_PORT:-/dev/ttyS0})
#DEVICE=16f88	# DON'T USE THIS - PICP MIS-PROGRAMS THE CHIP !!
DEVICE=16f819

# -----------------------------------------------------------------------------

FILES=README COPYING PARTS Makefile \
      waspic.sch waspic.pcb \
      BC337.sym pic16LF88.sym \
      waspic.asm 

CFLAGS=-Wall

.PHONY:		all prog filelist clean

define prog_pic
		picp $(PORT) $(DEVICE) -ef -bp -wp $^
endef

all:		waspic.hex

waspic.hex:	waspic.asm Makefile
		gpasm -o waspic.hex waspic.asm

prog:		waspic.hex
		$(prog_pic)

filelist:
		echo $(FILES)

clean:
		rm -f waspic.cod waspic.hex waspic.lst
		rm -f waspic.net waspic.new.pcb
