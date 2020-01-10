################################################################################
##
## Copyright (C) 2005-2008 Red Hat, Inc. All rights reserved.
##
## This copyrighted material is made available to anyone wishing to use,
## modify, copy, or redistribute it subject to the terms and conditions
## of the GNU General Public License v.2.
##
################################################################################

top_srcdir=../../../../..
UNINSTALL = ${top_srcdir}/scripts/uninstall.pl

include ${top_srcdir}/make/defines.mk

SNMP_CFLAGS = `net-snmp-config --cflags | sed -r 's/(-lcrypto|-Wdeclaration-after-statement)//g'`
SNMP_LDLAGS = `net-snmp-config --libs`

INCLUDE += -I ../include
CFLAGS += -fPIC $(SNMP_CFLAGS) -DPARANOIA=$(PARANOID)
CXXFLAGS += -fPIC $(SNMP_CFLAGS) -DPARANOIA=$(PARANOID)
LDFLAGS += -fPIC -shared $(SNMP_LDLAGS)

ifeq ($(PARANOID), 1)
	LDFLAGS += ${top_srcdir}/common/paranoid/*.o
else
	LDFLAGS += ${top_srcdir}/common/*.o
endif
LDFLAGS += ../common/*.o

LDFLAGS += -lcman

OBJECTS = clusterMonitorSnmp.o \
	clusterMIB.o \
	nodesMIB.o \
	nodesMIB_access.o \
	nodesMIB_checkfns.o \
	nodesMIB_checkfns_local.o \
	servicesMIB.o \
	servicesMIB_access.o \
	servicesMIB_checkfns.o \
	servicesMIB_checkfns_local.o

TARGET = libClusterMonitorSnmp.so


all: ${TARGET}

install:
	$(INSTALL_DIR) ${libdir}/cluster-snmp
	$(INSTALL_LIB) ${TARGET} ${libdir}/cluster-snmp

uninstall:
	${UNINSTALL} ${TARGET} ${libdir}/cluster-snmp

clean:
	rm -f $(OBJECTS) $(TARGET)

check:

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS)
