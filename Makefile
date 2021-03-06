#           
# Makefile - M8C utils, main Makefile
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

include Config.make


NAME=m8cutils

DIRS=shared regs libfdr das as
NONBUILD_DIRS=

ifeq ($(BUILD_LIBPROG),1)
    DIRS += prog bscan
else
    NONBUILD_DIRS += prog bscan
endif

DIRS += sim misc

SF_ACCOUNT=almesber@m8cutils.sourceforge.net
SF_DIR=/home/groups/m/m8/m8cutils/htdocs
SF_UPLOAD=$(SF_ACCOUNT):$(SF_DIR)

VERSION=$(shell cat VERSION)
DIR=$(NAME)-$(VERSION)
DISTFILE=$(NAME)-$(VERSION).tar.gz

ALL_DIRS=$(DIRS) $(NONBUILD_DIRS)
FILES=VERSION README CHANGES TODO COPYING.GPLv2 \
  Makefile Config.make Common.make \
  $(shell for n in $(ALL_DIRS); do $(MAKE) -s -C $$n filelist | \
    sed 's/^\| / '$$n'\//g'; done)
TESTS=\
  $(shell for n in $(DIRS); do $(MAKE) -s -C $$n testlist | \
    sed '/^$$/d;s/^\| / '$$n'\/tests\//g'; done)

.PHONY:	all clean spotless dep depend tests valgrind
.PHONY:	install uninstall dist upload


all:
		for n in $(DIRS); do $(MAKE) -C $$n all || exit 1; done

clean:
		for n in $(ALL_DIRS); do $(MAKE) -C $$n clean || exit 1; done

spotless:
		for n in $(ALL_DIRS); do $(MAKE) -C $$n spotless || exit 1; done

dep depend:
		for n in $(ALL_DIRS); do $(MAKE) -C $$n depend || exit 1; done

tests:		all
		passed=0 && for n in $(TESTS); do \
		  old_dir=`pwd` && cd `dirname $$n` && \
		  SCRIPT=$$n . ./`basename $$n` && cd $$old_dir; done; \
		  echo "Passed all $$passed tests" 2>&1

valgrind:
		VALGRIND="valgrind -q --leak-check=no" $(MAKE) tests

install:
		for n in $(ALL_DIRS); do $(MAKE) -C $$n install || exit 1; done

uninstall:
		for n in $(ALL_DIRS); do $(MAKE) -C $$n uninstall || exit 1; \
		  done
		-rmdir -p $(INSTALL_PREFIX)/share/m8cutils/include

dist:		$(FILES)
		rm -f $(DIR)
		ln -sf . $(DIR)
		tar cfz $(DISTFILE) $(FILES:%=$(DIR)/%)

upload:
		@echo -n "md5sum " && \
		  md5sum $(DISTFILE) | sed 's/ .*//'
		@echo -n "sha1sum " && \
		  sha1sum $(DISTFILE) | sed 's/ .*//'
		scp $(DISTFILE) $(SF_UPLOAD)               
		scp CHANGES $(SF_UPLOAD)/UTILS_CHANGES
		ssh $(SF_ACCOUNT) $(SF_DIR)/release.sh $(SF_DIR)
