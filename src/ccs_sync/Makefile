################################################################################
##
## Copyright (C) 2008-2009 Red Hat, Inc. All rights reserved.
##
## This copyrighted material is made available to anyone wishing to use,
## modify, copy, or redistribute it subject to the terms and conditions
## of the GNU General Public License v.2.
##
################################################################################

top_srcdir=..
UNINSTALL = ${top_srcdir}/scripts/uninstall.pl

include ../make/defines.mk

TARGET = ccs_sync

OBJECTS = \
	ricci_conf.o \
	ricci_list.o \
	ricci_nss.o \
	ricci_net.o \
	ricci_conf_xml.o

CC=gcc
INCLUDE = -I/usr/include/libxml2 -I/usr/include/nspr4 `nss-config --cflags`
CFLAGS = -O2 -Wall -Wextra -Wformat=2 -fstack-protector -Wshadow $(INCLUDE)
LDFLAGS = -lxml2 `nss-config --libs`

all: ${TARGET}

*.o: *.h ../include/*.h

install:
	$(INSTALL_DIR) ${bindir}
	$(INSTALL_BIN) ${TARGET} ${bindir}

uninstall:

clean:
	rm -f $(TARGET) $(OBJECTS)

check:

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
