.PHONY: clean
.PHONY: install
OPTIMIZATION := O2
LINKER_NAME = libsimpwire.so
SONAME := $(LINKER_NAME).0
TARGET := $(SONAME).1
CXX := g++
CXXFLAGS := -fPIC -std=c++11 -c -g -Wall -$(OPTIMIZATION)
LDFLAGS := -g -shared -fPIC -Wl,-soname,$(SONAME) -o $(TARGET) $(objs) -lc -pthread
CREATE_LINKS := ln -sf $(TARGET) $(SONAME) && ln -sf $(SONAME) $(LINKER_NAME) 
HEADER_INSTALL_LOCATION := /usr/include/simpwire/
LIBRARY_INSTALL_LOCATION := /usr/lib/

objs := TcpNode.o Peer.o 

all: $(TARGET) 

$(TARGET): $(objs)
	$(CXX) $(LDFLAGS) -o $@ $^
	$(CREATE_LINKS)

TcpNode.o: ../src/TcpNode.cpp ../include/TcpNode.hpp  Peer.o 
	$(CXX) $(CXXFLAGS) $<

Peer.o: ../src/Peer.cpp ../include/Peer.hpp
	$(CXX) $(CXXFLAGS) $<

install:
	mkdir -p $(HEADER_INSTALL_LOCATION)
	cp ../include/*.hpp $(HEADER_INSTALL_LOCATION)
	mkdir -p $(LIBRARY_INSTALL_LOCATION)
	cp $(TARGET) $(LIBRARY_INSTALL_LOCATION)
	ln -s $(LIBRARY_INSTALL_LOCATION)$(TARGET) $(LIBRARY_INSTALL_LOCATION)$(SONAME)
	ln -s $(LIBRARY_INSTALL_LOCATION)$(SONAME) $(LIBRARY_INSTALL_LOCATION)$(LINKER_NAME)

clean:
	rm -f $(objs) 

clear:
	rm -f $(objs) $(TARGET) $(SONAME) $(LINKER_NAME)
