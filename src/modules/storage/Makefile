################################################################################
##
## Copyright (C) 2005-2008 Red Hat, Inc. All rights reserved.
##
## This copyrighted material is made available to anyone wishing to use,
## modify, copy, or redistribute it subject to the terms and conditions
## of the GNU General Public License v.2.
##
################################################################################

top_srcdir=../..
UNINSTALL = ${top_srcdir}/scripts/uninstall.pl

include ${top_srcdir}/make/defines.mk


TARGET = ricci-modstorage

OBJECTS = main.o \
	FileMagic.o \
	Props.o \
	BD.o \
	Content.o \
	LV.o \
	PV.o \
	VG.o \
	LVM.o \
	Mapper.o \
	MapperSource.o \
	BDFactory.o \
	HD.o \
	StorageModule.o \
	ContentNone.o \
	ContentUnusable.o \
	ContentExtendedPartition.o \
	ContentFactory.o \
	FSController.o \
	ContentFS.o \
	ExtendedFS.o \
	GFS1.o \
	GFS2.o \
	UnsupportedFS.o \
	System.o \
	parted_wrapper.o \
	PartitionTable.o \
	Partition.o \
	PTSource.o \
	SwapFS.o \
	MapperFactory.o \
	MountHandler.o \
	MDRaidSource.o \
	mdadm_wrapper.o \
	MDRaid.o \
	MDRaidTarget.o


INCLUDE +=
CXXFLAGS += -DPARANOIA=$(PARANOID)
LDFLAGS += -lmagic

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
	$(INSTALL_FILE) d-bus/ricci-modstorage.oddjob.conf ${sysconfdir}/oddjobd.conf.d
	$(INSTALL_DIR) ${sysconfdir}/dbus-1/system.d
	$(INSTALL_FILE) d-bus/ricci-modstorage.systembus.conf ${sysconfdir}/dbus-1/system.d

uninstall:

clean:
	rm -f $(OBJECTS) $(TARGET)

check:

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

main_test: main_test.o mdadm_wrapper.o
	$(CXX) -o main_test main_test.o mdadm_wrapper.o $(LDFLAGS)
