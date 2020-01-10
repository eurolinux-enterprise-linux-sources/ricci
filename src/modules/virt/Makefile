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


TARGET = ricci-modvirt

OBJECTS = main.o \
	VirtModule.o \
	Virt.o

PARANOID=0
INCLUDE += -I${top_srcdir}/common/
CXXFLAGS += -DPARANOIA=$(PARANOID) -DVIRT_SUPPORT=$(VIRT_SUPPORT)

ifeq ($(VIRT_SUPPORT), 1)
	LDFLAGS += -lvirt
endif

ifeq ($(PARANOID), 1)
	LDFLAGS += ${top_srcdir}/common/paranoid/*.o
else
	LDFLAGS += ${top_srcdir}/common/*.o
endif

all: ${TARGET}

*.o: *.h

install:
	$(INSTALL_DIR) ${libexecdir}
	$(INSTALL_BIN) ${TARGET} ${libexecdir}
	$(INSTALL_DIR) ${sysconfdir}/oddjobd.conf.d
	$(INSTALL_FILE) d-bus/ricci-modvirt.oddjob.conf ${sysconfdir}/oddjobd.conf.d
	$(INSTALL_DIR) ${sysconfdir}/dbus-1/system.d
	$(INSTALL_FILE) d-bus/ricci-modvirt.systembus.conf ${sysconfdir}/dbus-1/system.d

uninstall:

clean:
	rm -f $(OBJECTS) $(TARGET)

check:

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
