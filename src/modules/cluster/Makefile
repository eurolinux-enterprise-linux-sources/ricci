################################################################################
##
## Copyright (C) 2005-2009 Red Hat, Inc. All rights reserved.
##
## This copyrighted material is made available to anyone wishing to use,
## modify, copy, or redistribute it subject to the terms and conditions
## of the GNU General Public License v.2.
##
################################################################################

top_srcdir=../..
UNINSTALL = ${top_srcdir}/scripts/uninstall.pl

include ${top_srcdir}/make/defines.mk


TARGET = modcluster

OBJECTS = main.o \
	ClusterModule.o \
	ClusterConf.o \
	ClusterStatus.o \
	Clusvcadm.o \
	Fence.o \
	XVM.o

PARANOID=0
INCLUDE += -I${top_srcdir}/common/
CXXFLAGS += -DPARANOIA=$(PARANOID)
LDFLAGS += -lcman -lfence

ifeq ($(PARANOID), 1)
	LDFLAGS += ${top_srcdir}/common/paranoid/*.o
else
	LDFLAGS += ${top_srcdir}/common/*.o
endif

all: ${TARGET}
	${MAKE} -C clumon all

*.o: *.h

install:
	$(INSTALL_DIR) ${libexecdir}
	$(INSTALL_BIN) ${TARGET} ${libexecdir}
	$(INSTALL_DIR) ${sysconfdir}/oddjobd.conf.d
	$(INSTALL_FILE) d-bus/modcluster.oddjob.conf ${sysconfdir}/oddjobd.conf.d
	$(INSTALL_DIR) ${sysconfdir}/dbus-1/system.d
	$(INSTALL_FILE) d-bus/modcluster.systembus.conf ${sysconfdir}/dbus-1/system.d
	${MAKE} -C clumon install

uninstall:

clean:
	rm -f $(OBJECTS) $(TARGET)
	${MAKE} -C clumon clean

check:

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
