.PHONY: clean

CXX := g++
CXXFLAGS := -std=c++11 -c -g -Wall 
LDFLAGS:= -g -Wall -pthread 
LIBS := -L../build -lsimpwire
TARGET := simpwire_demo

objs := demo.o

all: $(TARGET)

$(TARGET): $(objs)
	cd ../build && make
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)


demo.o : ../demo/demo.cpp 
	$(CXX) $(CXXFLAGS) $<

clean:
	rm -f $(objs) 

clear:
	rm -f $(objs) $(TARGET)
