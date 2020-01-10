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

ifeq ($(PEGASUS_PLATFORM),i386)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_IX86_GNU
endif

ifeq ($(PEGASUS_PLATFORM),i586)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_IX86_GNU
endif

ifeq ($(PEGASUS_PLATFORM),i686)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_IX86_GNU
endif

ifeq ($(PEGASUS_PLATFORM),athlon)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_IX86_GNU
endif

ifeq ($(PEGASUS_PLATFORM),ia64)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_IA64_GNU
endif

ifeq ($(PEGASUS_PLATFORM),x86_64)
# pegasus version < 2.5 doesn't define x86_64, it uses IA64 instead
# top_srcdir/configure redefines arch properly
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_X86_64_GNU
endif

ifeq ($(PEGASUS_PLATFORM),ppc)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_PPC_GNU
endif

ifeq ($(PEGASUS_PLATFORM),ppc64)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_PPC64_GNU
endif

ifeq ($(PEGASUS_PLATFORM),s390)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_ZSERIES_GNU
endif

ifeq ($(PEGASUS_PLATFORM),s390x)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_ZSERIES64_GNU
endif

ifeq ($(PEGASUS_PLATFORM),sparc)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_SPARCV9_GNU
endif

ifeq ($(PEGASUS_PLATFORM),sparc64)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_SPARC64_GNU
endif

ifeq ($(PEGASUS_PLATFORM),sh4)
	PEGASUS_CXXFLAGS = -DPEGASUS_PLATFORM_LINUX_SH4_GNU
endif

INCLUDE += -I ../include
CXXFLAGS += $(PEGASUS_CXXFLAGS) -DPARANOIA=$(PARANOID)
LDFLAGS += -shared -ldl -lcrypt -lcman

ifeq ($(PARANOID), 1)
	LDFLAGS += ${top_srcdir}/common/paranoid/*.o
else
	LDFLAGS += ${top_srcdir}/common/*.o
endif

OBJECTS = ClusterProviderMain.o ClusterProvider.o

TARGET = libRedHatClusterProvider.so
TARGET_TEST = clusterCIM_test


all: ${TARGET}

install:
	$(INSTALL_DIR) ${pegasus_providers_dir}
	$(INSTALL_LIB) ${TARGET} ${pegasus_providers_dir}

uninstall:
	${UNINSTALL} ${TARGET} ${pegasus_providers_dir}

clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET_TEST)

check:

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) 

$(TARGET_TEST): clusterCIM_test.*
	$(CXX) $(CXXFLAGS) -lpegcommon -lpegclient -lpthread -lcrypt -o $@ $@.cpp
