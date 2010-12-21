#
# Makefile - Makefile of Werner's slow PIC-based programmer
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

# For now, Common.make is not useful here, so we don't include it


# ----- User-configurable items -----------------------------------------------

PORT = $(shell echo $${PICSTART_PORT:-/dev/ttyS0})
#DEVICE=16f88	# DON'T USE THIS - PICP MIS-PROGRAMS THE CHIP !!
DEVICE=16f819

# -----------------------------------------------------------------------------

FILES=README COPYING PARTS Makefile \
      project waspic.sch waspic.pcb \
      waspic.asm 

.PHONY:		all prog dep depend tests filelist clean spotless

define prog_pic
		picp $(PORT) $(DEVICE) -ef -bp -wp $^
endef


all:		waspic.hex

waspic.hex:	waspic.asm Makefile
		gpasm -o waspic.hex waspic.asm

prog:		waspic.hex
		$(prog_pic)

# ----- Dependencies (dummy target) -------------------------------------------

dep depend:

# ----- Tests -----------------------------------------------------------------

tests:

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f waspic.cod waspic.hex waspic.lst
		rm -f waspic.net waspic.new.pcb

spotless:	clean
		rm -f waspic.hex
