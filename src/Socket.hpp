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

#ifndef SPW_SOCKET_HPP_
#define SPW_SOCKET_HPP_

#include "../include/common.hpp"
#include "ISocket.hpp"

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace spw
{

constexpr uint32_t SPW_DEF_SLEEPTIME_MS = 10;
constexpr size_t SPW_DEF_RECBUF_SIZE = 1024;


class Socket : public ISocket
{
public:

    Socket();
    virtual ~Socket() override;

    bool listen(
        uint16_t port, 
        IpVersion version = IpVersion::ANY) override;

    bool connect(
        const std::string &ip, 
        uint16_t port) override;

    void close() override;

    ISocket* accept() override;

    ReceiveResult receive(
        std::vector<uint8_t> &receiveData) override;

    size_t send(
        const std::vector<uint8_t> &dataToSend) override;

    int32_t socketNumber() override;
    bool isListener() override;
    uint16_t listenPort() override;
    bool isConnected() override;
    bool isListening() override;
    std::string peerIpAddress() override;
    uint16_t peerPort() override;
    std::string ownIpAddress() override;
    std::string peerName() override;
    void setReceiveBufferSize(size_t newSize) override;
    size_t receiveBufferSize() override;
    void setSleepTime(uint32_t milliseconds) override;
    uint32_t sleepTime() override;
    int getLastErrno() override;
    std::string getLastErrnoString() override;

protected:

    virtual void setErrno();
    virtual void clearErrno();

private:

#ifdef _WIN32
    static void winsockInit();
    static void winsockDeInit();
    static uint64_t m_socket_counter;
    static WSAData m_wsadata;
#endif

    int m_socket_fd = -1;
    bool m_is_listener = false;
    bool m_is_connected = false;
    bool m_is_listening = false;
    uint16_t m_listen_port = 0;
    size_t m_receive_buffer_size = SPW_DEF_RECBUF_SIZE;
    uint32_t m_sleep_time = SPW_DEF_SLEEPTIME_MS;
    int m_last_errno = 0;

};

}

#endif //_SPW_SOCKET_HPP
