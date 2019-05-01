#ifndef SPW_I_SOCKET_HPP_
#define SPW_I_SOCKET_HPP_

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

#include <cstdint>
#include <string>
#include <vector>
#include "../include/common.hpp"


namespace spw
{

class ISocket
{
public:

    enum class ReceiveResult {
        OK,
        ERROR_NO_CONNECTION,
        ERROR_PEER_DISCONNECTED,
        ERROR_IS_LISTENER,
        ERROR_NOTHING_RECEIVED,
        ERROR_SYSTEM
    };


    virtual ~ISocket() = default;

    virtual bool listen(
            uint16_t port, 
            IpVersion version) = 0;

    virtual bool connect(
            const std::string &ip, 
            uint16_t port) = 0;

    virtual void close() = 0;

    virtual ISocket* accept() = 0;

    virtual ReceiveResult receive(
            std::vector<uint8_t> &receivedData) = 0;

    virtual size_t send(
            const std::vector<uint8_t> &dataToSend) = 0;

    virtual int32_t socketNumber() = 0;
    virtual bool isListener() = 0;
    virtual uint16_t listenPort() = 0;
    virtual bool isConnected() = 0;
    virtual bool isListening() = 0;
    virtual std::string peerIpAddress() = 0;
    virtual uint16_t peerPort() = 0;
    virtual std::string ownIpAddress() = 0;
    virtual std::string peerName() = 0;
    virtual void setReceiveBufferSize(size_t newSize) = 0;
    virtual size_t receiveBufferSize() = 0;
    virtual void setSleepTime(uint32_t milliseconds) = 0;
    virtual uint32_t sleepTime() = 0;
    virtual int getLastErrno() = 0;
    virtual std::string getLastErrnoString() = 0;


protected:

private:

};

}

#endif //SPW_I_SOCKET_HPP_
