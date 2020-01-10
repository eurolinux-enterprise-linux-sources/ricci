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

TARGET = libClusterMonitor.a

OBJECTS = Cluster.o Node.o Service.o ClusterMonitor.o

INCLUDE += -I ../include
CXXFLAGS += -DPARANOIA=$(PARANOID) -fPIC

all: ${TARGET}

install:

uninstall:

clean:
	rm -f $(OBJECTS) $(TARGET)

check:

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(AR) rc $(TARGET) $(OBJECTS)
	$(RANLIB) $(TARGET)
