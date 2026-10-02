# Copyright (c) 2019, AT&T Intellectual Property.
# All rights reserved.
#
# SPDX-License-Identifier: LGPL-2.1-only

TARGET := vci.so
VENDORLIB := $(shell perl -MConfig -e 'print %Config{"vendorlib"} . ""')
VENDORARCH := $(shell perl -MConfig -e 'print %Config{"vendorarch"} . ""')/auto/vci

all: $(TARGET)
	install -d dist$(VENDORLIB)
	install -d dist$(VENDORARCH)
	install --mode=644 -D vci.pm dist/$(VENDORLIB)
	install --mode=644 -D $(TARGET) dist/$(VENDORARCH)

vci_wrap.cxx: vci.i
	swig -Wall -c++ -perl5 vci.i

$(TARGET): vci_wrap.cxx helpers.hpp ../../vci.hpp
	g++ $(CPPFLAGS) $(CXXFLAGS) $(shell perl -MExtUtils::Embed -e ccopts) -L../../ -shared -fPIC $(LDFLAGS) -Wl,-soname,$(TARGET) -o $(TARGET) -std=c++11 vci_wrap.cxx -lvci


clean:
	rm -f $(TARGET)
	rm -f vci.pm
	rm -f vci_wrap.cxx
	rm -f vci_wrap.h
	rm -rf dist

