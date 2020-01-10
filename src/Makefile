################################################################################
##
## Copyright (C) 2005-2008 Red Hat, Inc. All rights reserved.
##
## This copyrighted material is made available to anyone wishing to use,
## modify, copy, or redistribute it subject to the terms and conditions
## of the GNU General Public License v.2.
##
################################################################################

include make/version.in
include make/defines.mk

.PHONY: ricci

all: ricci clustermon

ricci:
	${MAKE} -C common
	${MAKE} -C ricci
	${MAKE} -C modules
	${MAKE} -C init.d
	${MAKE} -C pam.d
	${MAKE} -C ccs_sync

clustermon:
	${MAKE} -C common
	${MAKE} -C modules clustermon

install: install_ricci install_clustermon

install_ricci:
	${INSTALL_DIR} -m 700 ${localstatedir}/lib/ricci
	${MAKE} -C ricci install
	${MAKE} -C modules install
	${MAKE} -C init.d install
	${MAKE} -C pam.d install
	${MAKE} -C ccs_sync install
	${INSTALL_DIR} ${docdir}/ricci-${VERSION}/
	${MAKE} -C docs install
	${INSTALL_FILE} COPYING ${docdir}/ricci-${VERSION}/

install_clustermon:
	${MAKE} -C modules install_clustermon
	${INSTALL_DIR} ${docdir}/modcluster-${VERSION}/
	${INSTALL_FILE} COPYING ${docdir}/modcluster-${VERSION}/

uninstall:

clean:
	${MAKE} -C common clean
	${MAKE} -C ricci clean
	${MAKE} -C ccs_sync clean
	${MAKE} -C modules clean
	${MAKE} -C init.d clean
	${MAKE} -C pam.d clean

clean_clustermon:
	${MAKE} -C modules clean

distclean: clean
	rm -f make/defines.mk

check:

rebuild: clean all
