/*
Copyright (c) 2019 Ivan Brebric

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "gmock/gmock.h"
#include "../src/ISocket.hpp"

class MockSocket : public spw::ISocket
{
  public:
    MOCK_METHOD2(listen, bool(uint16_t port, spw::IpVersion version));
    MOCK_METHOD2(connect, bool(const std::string &ip, uint16_t port));
    MOCK_METHOD0(close, void());
    MOCK_METHOD0(accept, spw::ISocket*());
    MOCK_METHOD1(
      receive, 
      spw::ISocket::ReceiveResult(std::vector<uint8_t> &receiveData));
    MOCK_METHOD1(
      send, 
      size_t(const std::vector<uint8_t> &dataToSend));
    MOCK_METHOD0(socketNumber, int32_t());
    MOCK_METHOD0(isListener, bool());
    MOCK_METHOD0(listenPort, uint16_t());
    MOCK_METHOD0(isConnected, bool());
    MOCK_METHOD0(isListening, bool());
    MOCK_METHOD0(peerIpAddress, std::string());
    MOCK_METHOD0(peerPort, uint16_t());
    MOCK_METHOD0(ownIpAddress, std::string());
    MOCK_METHOD0(peerName, std::string());
    MOCK_METHOD1(setReceiveBufferSize, void(size_t newSize));
    MOCK_METHOD0(receiveBufferSize, size_t());
    MOCK_METHOD1(setSleepTime, void(uint32_t milliseconds));
    MOCK_METHOD0(sleepTime, uint32_t());
    MOCK_METHOD0(getLastErrno, int());
    MOCK_METHOD0(getLastErrnoString, std::string());
};
