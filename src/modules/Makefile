################################################################################
##
## Copyright (C) 2005-2008 Red Hat, Inc. All rights reserved.
##
## This copyrighted material is made available to anyone wishing to use,
## modify, copy, or redistribute it subject to the terms and conditions
## of the GNU General Public License v.2.
##
################################################################################

.PHONY: clustermon

all:
	${MAKE} -C storage
	${MAKE} -C rpm
	${MAKE} -C log
	${MAKE} -C service
	${MAKE} -C virt

clustermon:
	${MAKE} -C cluster

install:
	${MAKE} -C storage install
	${MAKE} -C rpm install
	${MAKE} -C log install
	${MAKE} -C service install
	${MAKE} -C virt install

install_clustermon:
	${MAKE} -C cluster install

uninstall:

clean:
	${MAKE} -C storage clean
	${MAKE} -C cluster clean
	${MAKE} -C rpm clean
	${MAKE} -C log clean
	${MAKE} -C service clean
	${MAKE} -C virt clean

check:

rebuild: clean all
