.PHONY: clean

CXX := g++
CXXFLAGS := -std=c++11 -c -g -Wall 
LDFLAGS := -g
TARGET := simpwire_demo

objs := TcpEndpoint.o Peer.o demo.o


all: $(TARGET)

$(TARGET): $(objs)
	$(CXX) $(LDFLAGS) -o $@ $^

demo.o: ../demo/demo.cpp ../src/TcpEndpoint.h
	$(CXX) $(CXXFLAGS) $<

TcpEndpoint.o: ../src/TcpEndpoint.cpp ../src/TcpEndpoint.h ../src/Peer.h ../src/Exceptions.h
	$(CXX) $(CXXFLAGS) $<

Peer.o: ../src/Peer.cpp ../src/Peer.h
	$(CXX) $(CXXFLAGS) $<

clean:
	rm -f $(objs)
