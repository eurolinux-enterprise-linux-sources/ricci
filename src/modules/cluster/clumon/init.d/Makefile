################################################################################
##
## Copyright (C) 2005-2008 Red Hat, Inc. All rights reserved.
##
## This copyrighted material is made available to anyone wishing to use,
## modify, copy, or redistribute it subject to the terms and conditions
## of the GNU General Public License v.2.
##
################################################################################

top_srcdir=../../../..
UNINSTALL=${top_srcdir}/scripts/uninstall.pl

include ${top_srcdir}/make/defines.mk

TARGET = modclusterd

all:

clean:

install:
	$(INSTALL_DIR) ${sysconfdir}/rc.d/init.d
	$(INSTALL_BIN) ${TARGET} ${sysconfdir}/rc.d/init.d

uninstall:
	${UNINSTALL} ${TARGET} ${DESTDIR}/etc/init.d
