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

TARGET = modclusterd

OBJECTS = main.o \
	Monitor.o \
	Peer.o \
	Communicator.o

INCLUDE += -I../include
CXXFLAGS += -DPARANOIA=$(PARANOID)
LDFLAGS += ../common/*.o

ifeq ($(PARANOID), 1)
	LDFLAGS += ${top_srcdir}/common/paranoid/*.o
else
	LDFLAGS += ${top_srcdir}/common/*.o
endif

LDFLAGS += -lcman

all: ${TARGET}

install:
	$(INSTALL_DIR) ${sbindir}
	$(INSTALL_BIN) ${TARGET} ${sbindir}

uninstall:
	${UNINSTALL} ${TARGET} ${sbindir}

clean:
	rm -f $(OBJECTS) $(TARGET)

check:

rebuild: clean all

*.o: *.h

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
