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

TARGET = ricci
TARGET_WORKER = ricci-worker

OBJECTS = main.o \
	ClientInstance.o \
	Server.o \
	DBusController.o \
	SSLInstance.o \
	Ricci.o \
	Auth.o \
	QueueLocker.o

TARGET_WORKER_OBJECTS = RicciWorker.o \
	DBusController.o \
	QueueLocker.o \
	RebootModule.o


#OBJECTS = dbus_test.o
#OBJECTS = ssl_test.o

PARANOID=1

INCLUDE += `pkg-config --cflags dbus-1` -I../common
CFLAGS +=
CXXFLAGS += -DPARANOIA=$(PARANOID)
LDFLAGS += -lcap `pkg-config --libs dbus-1`

ifeq ($(PARANOID), 1)
	LDFLAGS += ${top_srcdir}/common/paranoid/*.o
else
	LDFLAGS += ${top_srcdir}/common/*.o
endif

all: ${TARGET} ${TARGET_WORKER}

*.o: *.h ../include/*.h

install:
	$(INSTALL_DIR) ${sbindir}
	$(INSTALL_BIN) ${TARGET} ${sbindir}
	$(INSTALL_DIR) ${libexecdir}/ricci
	$(INSTALL_BIN) ${TARGET_WORKER} ${libexecdir}/ricci
	$(INSTALL_DIR) ${localstatedir}/lib/ricci/queue
	$(INSTALL_DIR) ${localstatedir}/lib/ricci/certs
	$(INSTALL_FILE) cacert.config ${localstatedir}/lib/ricci/certs/
	$(INSTALL_DIR) ${localstatedir}/lib/ricci/certs/clients
	$(INSTALL_DIR) ${sysconfdir}/oddjobd.conf.d
	$(INSTALL_FILE) d-bus/ricci.oddjob.conf ${sysconfdir}/oddjobd.conf.d
	$(INSTALL_DIR) ${sysconfdir}/dbus-1/system.d
	$(INSTALL_FILE) d-bus/ricci.systembus.conf ${sysconfdir}/dbus-1/system.d

uninstall:

clean:
	rm -f $(TARGET) $(OBJECTS)
	rm -f $(TARGET_WORKER) $(TARGET_WORKER_OBJECTS)

check:

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS) -lsasl2

${TARGET_WORKER}: ${TARGET_WORKER_OBJECTS}
	$(CXX) -o ${TARGET_WORKER} ${TARGET_WORKER_OBJECTS} ${LDFLAGS}
