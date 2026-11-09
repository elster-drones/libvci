# Copyright (c) 2018-2019, AT&T Intellectual Property.
# All rights reserved.
#
# SPDX-License-Identifier: LGPL-2.1-only

TARGET := libvci.so.1
TARGET_LINK := libvci.so
PYTHON3_LIB := swig/python3/_vci.so
PERL5_LIB := swig/perl5/libvci-perl5.so.1

GOBUILDFLAGS := -buildmode=c-archive
CFLAGS += -fPIC
CXXFLAGS += -fPIC

SOURCES := $(wildcard *.c)
CPP_SOURCES := $(wildcard *.cpp)
GO_SOURCES := $(wildcard go-vci-interface/*.go)

GENERATED_OBJS := $(patsubst %.c,%.o,$(SOURCES))
CPP_GENERATED_OBJS := $(patsubst %.cpp,%.oxx,$(CPP_SOURCES))

GO_HEADER := cgo-export/vci-interface.h
GO_LIB := cgo-export/vci-interface.a

all: $(TARGET) $(TARGET_LINK) $(PYTHON3_LIB) $(PERL5_LIB) vci.pc

$(GO_LIB): $(GO_SOURCES)
	go build $(GOBUILDFLAGS) -o $(GO_LIB) ./cgo-export

$(GO_HEADER): $(GO_LIB)

%.o: %.c %.h $(GO_HEADER)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.oxx: %.cpp %.hpp %.h $(GO_HEADER)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(TARGET): $(GENERATED_OBJS) $(CPP_GENERATED_OBJS) $(GO_LIB)
	$(CXX) -shared -fPIC $(LDFLAGS) -Wl,-soname,$@ -o $@ -lpthread $^

$(TARGET_LINK): $(TARGET)
	ln -s $(TARGET) $(TARGET_LINK)

$(PYTHON3_LIB): $(TARGET_LINK) vci.hpp vci.h
	$(MAKE) -C swig/python3

$(PERL5_LIB): $(TARGET_LINK) vci.hpp vci.h
	$(MAKE) -C swig/perl5

examples/c/vci-c-example: examples/c/main.c vci.h $(TARGET_LINK)
	$(CC) -L. -I. -std=gnu11 -o $@ $< -lvci

examples/c++/vci-c++-example: examples/c++/main.cpp vci.hpp vci.h $(TARGET_LINK)
	$(CXX) -L. -I. -std=c++11 -o $@ $< -lvci

examples/go/vci-go-example: examples/go/main.go
	go build -o $@ $<

vci.pc:
	echo 'prefix=/usr' >> $@
	echo 'exec_prefix=$${prefix}' >> $@
	echo 'libdir=$${exec_prefix}/lib/$(DEB_HOST_MULTIARCH)' >> $@
	echo 'includedir=$${exec_prefix}/include' >> $@
	echo '' >> $@
	echo 'Name: vci' >> $@
	echo 'Description: VCI library' >> $@
	echo 'Version: 1.0.0' >> $@
	echo 'Libs: -L$${libdir} -lvci' >> $@
	echo 'Cflags: -I$${includedir}' >> $@

clean:
	$(MAKE) -C swig/python3 clean
	rm -f $(GENERATED_OBJS)
	rm -f $(CPP_GENERATED_OBJS)
	rm -f $(GO_LIB)
	rm -f $(GO_HEADER)
	rm -f $(TARGET)
	rm -f $(TARGET_LINK)
	rm -f vci.pc
	rm -f examples/c/vci-c-example
	rm -f examples/c++/vci-c++-example
	rm -f examples/go/vci-go-example
