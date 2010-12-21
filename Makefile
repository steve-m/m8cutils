#           
# Makefile - M8C utils, main Makefile
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

DIR=m8c
NAME=m8cutils
DIRS=shared libfdr das as prog sim waspic watsp

SF_ACCOUNT=almesber@m8cutils.sourceforge.net
SF_DIR=/home/groups/m/m8/m8cutils/htdocs         
SF_UPLOAD=$(SF_ACCOUNT):$(SF_DIR)

VERSION=$(shell cat VERSION)
DISTFILE=$(NAME)-$(VERSION).tar.gz

FILES=VERSION README CHANGES TODO COPYING.GPLv2 \
  Makefile Common.make \
  $(shell for n in $(DIRS); do $(MAKE) -s -C $$n filelist | \
    sed 's/^\| / '$$n'\//g'; done)

.PHONY:	all clean spotless dep depend tests dist upload


all:
		for n in $(DIRS); do $(MAKE) -C $$n || exit 1; done

clean:
		for n in $(DIRS); do $(MAKE) -C $$n clean || exit 1; done

spotless:
		for n in $(DIRS); do $(MAKE) -C $$n spotless || exit 1; done

dep depend:
		for n in $(DIRS); do $(MAKE) -C $$n depend || exit 1; done

tests:
		for n in $(DIRS); do $(MAKE) -C $$n tests || exit 1; done

dist:		$(FILES)
		cd ..; tar cfz $(DIR)/$(DISTFILE) $(FILES:%=$(DIR)/%)

upload:		dist
		scp $(DISTFILE) host:/home/httpd/almesberger/misc/cy8c2/

notyet:
		@echo -n "md5sum " && \
		  md5sum $(DISTFILE) | sed 's/ .*//'
		@echo -n "sha1sum " && \
		  sha1sum $(DISTFILE) | sed 's/ .*//'
		scp $(DISTFILE) $(SF_UPLOAD)               
		scp CHANGES $(SF_UPLOAD)
		ssh $(SF_ACCOUNT) $(SF_DIR)/release.sh $(SF_DIR)
