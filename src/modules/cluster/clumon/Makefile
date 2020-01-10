################################################################################
##
## Copyright (C) 2005-2008 Red Hat, Inc. All rights reserved.
##
## This copyrighted material is made available to anyone wishing to use,
## modify, copy, or redistribute it subject to the terms and conditions
## of the GNU General Public License v.2.
##
################################################################################

top_srcdir=../../..
UNINSTALL=${top_srcdir}/scripts/uninstall.pl

include ${top_srcdir}/make/version.in
include ${top_srcdir}/make/defines.mk

all:
	${MAKE} -C src all

clean:
	${MAKE} -C src clean

install:
	${MAKE} -C src install
	${MAKE} -C init.d install
	${MAKE} -C man install

	# snmp
	$(INSTALL_DIR) ${datadir}/snmp/mibs
	$(INSTALL_FILE) REDHAT-MIB ${datadir}/snmp/mibs/
	$(INSTALL_FILE) REDHAT-CLUSTER-MIB ${datadir}/snmp/mibs/
	$(INSTALL_DIR) ${docdir}/cluster-snmp-${VERSION}
	$(INSTALL_FILE) README ${docdir}/cluster-snmp-${VERSION}/
	$(INSTALL_FILE) README.snmpd ${docdir}/cluster-snmp-${VERSION}/
	$(INSTALL_FILE) COPYING ${docdir}/cluster-snmp-${VERSION}/
	$(INSTALL_FILE) REDHAT-MIB ${docdir}/cluster-snmp-${VERSION}/
	$(INSTALL_FILE) REDHAT-CLUSTER-MIB ${docdir}/cluster-snmp-${VERSION}/

	# cim
	$(INSTALL_DIR) ${docdir}/cluster-cim-${VERSION}
	$(INSTALL_FILE) README ${docdir}/cluster-cim-${VERSION}/
	$(INSTALL_FILE) README.cim ${docdir}/cluster-cim-${VERSION}/
	$(INSTALL_FILE) COPYING ${docdir}/cluster-cim-${VERSION}/
	$(INSTALL_FILE) RedHat_ClusterProvider.mof ${docdir}/cluster-cim-${VERSION}/
	$(INSTALL_FILE) RedHat_ClusterSchema.mof ${docdir}/cluster-cim-${VERSION}/

uninstall:
	${MAKE} -C src uninstall
	${MAKE} -C man uninstall
	${MAKE} -C init.d uninstall
	${UNINSTALL} REDHAT-MIB ${DESTDIR}/usr/share/snmp/mibs/
	${UNINSTALL} REDHAT-CLUSTER-MIB ${DESTDIR}/usr/share/snmp/mibs/

distclean: clean
	rm -f make/defines.mk

check:
	${MAKE} -C src check

rebuild: clean all
