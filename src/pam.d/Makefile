################################################################################
##
## Copyright (C) 2005-2009 Red Hat, Inc. All rights reserved.
##
## This copyrighted material is made available to anyone wishing to use,
## modify, copy, or redistribute it subject to the terms and conditions
## of the GNU General Public License v.2.
##
################################################################################

top_srcdir=..
UNINSTALL = ${top_srcdir}/scripts/uninstall.pl

include ${top_srcdir}/make/defines.mk

all:

install:
	$(INSTALL_DIR) ${sysconfdir}/pam.d
	$(INSTALL_FILE) ricci ${sysconfdir}/pam.d/

uninstall:

clean:

check:

rebuild: clean all
