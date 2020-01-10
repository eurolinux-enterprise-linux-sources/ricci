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

OBJECTS = \
	Except.o \
	executils.o \
	signals.o \
	Thread.o \
	Time.o \
	utils.o \
	File.o \
	XML.o \
	Network.o \
	Socket.o \
	ServerSocket.o \
	ClientSocket.o \
	Logger.o \
	Variable.o \
	Random.o \
	Module.o \
	daemon_init.o \
	base64.o \
	sys_util.o

INCLUDE +=
CXXFLAGS +=
CXXFLAGS_PARANOIA = -DPARANOIA=1
CFLAGS_PARANOIA = -DPARANOIA=1
CFLAGS +=
LDFLAGS +=

.PHONY: prep

all: prep compile

compile: $(OBJECTS)

.cpp.o:
	$(CXX) $(INCLUDE) $(CPPFLAGS) $(CXXFLAGS) -c $<
	$(CXX) $(INCLUDE) $(CPPFLAGS) $(CXXFLAGS) $(CXXFLAGS_PARANOIA) -c $< -o ${top_srcdir}/common/paranoid/$@

.c.o:
	$(CC) $(INCLUDE) $(CPPFLAGS) $(CFLAGS) -c $<
	$(CC) $(INCLUDE) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_PARANOIA) -c $< -o ${top_srcdir}/common/paranoid/$@

install:

uninstall:

clean:
	rm -rf ${top_srcdir}/common/paranoid/
	rm -f $(OBJECTS)

prep:
	@mkdir ${top_srcdir}/common/paranoid/ 2>/dev/null ||:

check:

rebuild: clean all
