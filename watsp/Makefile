#
# Makefile - Makefile of Werner's trivial serial programmer
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

# For now, Common.make is not useful here, so we don't include it


# -----------------------------------------------------------------------------

FILES=README PARTS Makefile \
      project watsp.sch watsp.pcb

.PHONY:		all dep depend tests filelist clean spotless

all:

# ----- Dependencies (dummy target) -------------------------------------------

dep depend:

# ----- Tests -----------------------------------------------------------------

tests:

# ----- Distribution ----------------------------------------------------------

filelist:
		@echo $(FILES)

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f watsp.net watsp.new.pcb

spotless:	clean
