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

#include "Socket.hpp"

#include <cstring>
#include <thread>

#ifdef __linux__

#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#elif defined _WIN32

#ifdef __MINGW32__
#include <w32api.h>
#else
#define Windows7 0x601
#endif

#define _WIN32_WINNT Windows7
#include <winsock2.h>
#include <ws2tcpip.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

namespace spw
{

#ifdef _WIN32
uint64_t Socket::m_socket_counter = 0;
WSADATA Socket::m_wsadata;
#endif

static void closeSocket(int sockfd)
{
#ifdef __linux__
    ::close(sockfd);
#elif _WIN32
    closesocket(sockfd);
#endif
}

static bool setNonBlocking(int sockfd)
{
    bool success = false;

#ifdef __linux__

    if(sockfd != -1)
    {
        long socket_settings = fcntl(sockfd, F_GETFL, NULL);

        if(socket_settings != -1)
        {
            socket_settings |= O_NONBLOCK;
            if(fcntl(sockfd, F_SETFL, socket_settings) != -1)
            {
                success = true;
            }
        }
    }

#elif _WIN32
    u_long nonblocking = 1;
    success = ioctlsocket(sockfd, FIONBIO, &nonblocking) == 0;
#endif

    return success;
}


Socket::Socket()
{
#ifdef _WIN32
    if(m_socket_counter == 0)
    {
        winsockInit();
    }
    ++m_socket_counter;
#endif
}

Socket::~Socket()
{
    this->close();

#ifdef _WIN32
    if(m_socket_counter == 1)
    {
        winsockDeInit();
    }
    --m_socket_counter;
#endif
}

bool Socket::listen(
        uint16_t port, 
        IpVersion version)
{
    bool success = false;
    addrinfo hints;
    addrinfo *result;

    this->close();

    //Make sure port cannot be 0
    if(port == 0)
    {
        return false;
    }

    std::memset(&hints, 0, sizeof(hints));

    if(version == IpVersion::ANY)
    {
        hints.ai_family = AF_UNSPEC;
    }
    else if(version == IpVersion::IPV4)
    {
        hints.ai_family = AF_INET;
    }
    else if(version == IpVersion::IPV6)
    {
        hints.ai_family = AF_INET6;
    }

    hints.ai_socktype = SOCK_STREAM;//TCP
    hints.ai_flags = AI_PASSIVE; //localhost

    if(getaddrinfo(
                NULL,
                std::to_string(port).c_str(),
                &hints,
                &result) == 0)
    {
        //Store pointer to first element
        //(for later memory freeing)
        addrinfo *p_first = result;

        while(result != NULL)
        {
            m_socket_fd = socket(
                    result->ai_family,
                    result->ai_socktype,
                    result->ai_protocol);

#ifdef __linux__
            
            if(m_socket_fd != -1)
            {
                int yes = 1;

                if(setsockopt(
                            m_socket_fd,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            &yes, sizeof(yes)) == 0 &&
                     bind(
                             m_socket_fd,
                             result->ai_addr,
                             result->ai_addrlen) == 0 &&
                     ::listen(m_socket_fd, 20) == 0)
                {
                    if(setNonBlocking(m_socket_fd))
                    {
                        success = true;
                        m_is_listener = true;
                        m_listen_port = port;
                        break;
                    }
                }
                else
                {
                    closeSocket(m_socket_fd);
                    m_socket_fd = -1;
                }
            }

#elif _WIN32

            if(m_socket_fd != static_cast<int>(INVALID_SOCKET))
            {
                int yes = 1;

                if(setsockopt(
                            m_socket_fd,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            (const char*)&yes, sizeof(yes)) == 0 &&
                     bind(
                            m_socket_fd,
                            result->ai_addr,
                            result->ai_addrlen) == 0 &&
                     ::listen(m_socket_fd, 20) == 0 )
                {
                    if(setNonBlocking(m_socket_fd))
                    {
                        success = true;
                        m_is_listener = true;
                        m_listen_port = port;
                        //setNonBlocking(m_socket_fd);
                        break;
                    }
                    else
                    {
                        closeSocket(m_socket_fd);
                        m_socket_fd = -1;
                    }
                }
                else
                {
                    closeSocket(m_socket_fd);
                    m_socket_fd = -1;
                }
            }
#endif

            result = result->ai_next;
        }

        freeaddrinfo(p_first);
    }

    if(!success)
    {
        setErrno();
    }
    else
    {
        m_is_listener = true;
        m_is_listening = true;
        clearErrno();
    }

    return success;
}

bool Socket::connect(
    const std::string &ip, 
    uint16_t port)
{
    bool success = false;
    this->close();

    addrinfo hints;
    addrinfo *result = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    std::string hostport_str = std::to_string(port);

    if(getaddrinfo(ip.c_str(),
         hostport_str.c_str(),
         &hints, &result) == 0)
    {
        if(result)
        {
            m_socket_fd = socket(
                                        result->ai_family, 
                                        result->ai_socktype,
                                        result->ai_protocol);
            
            if(setNonBlocking(m_socket_fd))
            {
                int conn = ::connect(m_socket_fd, result->ai_addr, result->ai_addrlen);
                if(conn == 0)
                {
                    success = true;
                }
#ifdef __linux__
                else if(errno == EINPROGRESS)
                {
                    std::this_thread::sleep_for(
                            std::chrono::milliseconds(sleepTime()));
                        
                    int sock_error;
                    socklen_t len = sizeof(sock_error);

                    if(getsockopt(m_socket_fd,SOL_SOCKET,SO_ERROR,
                                                (void*)&sock_error, &len) == 0)
                    {
                        if(sock_error == 0)
                        { 
                            success = true;
                        }
                    }
                }
#elif _WIN32
                else if(WSAGetLastError() == WSAEWOULDBLOCK)
                {
                    std::this_thread::sleep_for(
                            std::chrono::milliseconds(sleepTime()));

                    conn = ::connect(m_socket_fd, result->ai_addr, result->ai_addrlen);
                    int last_error = WSAGetLastError();
                    int optval;
                    int len = sizeof(optval);

                    if((conn == 0 || last_error == WSAEISCONN) &&
                         getsockopt(m_socket_fd, SOL_SOCKET, SO_ERROR,
                                                (char*)&optval, &len) == 0)
                    {
                        if(optval == 0)
                        {
                            success = true;
                        }
                    }
                }
#endif
            }

            freeaddrinfo(result);
        }
    }

    if(!success)
    {
        this->close();
        setErrno();
    }
    else
    {
        m_is_connected = true;
        clearErrno();
    }

    return success;
}

void Socket::close()
{
    m_is_connected = false;
    m_is_listening = false;
    m_is_listener = false;
    closeSocket(m_socket_fd);
    m_socket_fd = -1;
}

ISocket* Socket::accept()
{
    ISocket *result = nullptr;

    if(isListener())
    {
        sockaddr_storage peer_addr;
        socklen_t addr_size = sizeof(peer_addr);
        int peer_socket_fd = 
                ::accept(m_socket_fd,
                             (sockaddr*)&peer_addr,
                             &addr_size);

        if(peer_socket_fd != -1)
        {
            Socket *newSocket = new Socket();
            setNonBlocking(peer_socket_fd);
            newSocket->m_socket_fd = peer_socket_fd;
            newSocket->m_is_connected = true;
            result = newSocket;
        }
        else
        {
            setErrno();
            int errornum = getLastErrno();
#ifdef __linux__
            if(errornum == EAGAIN || errornum == EWOULDBLOCK)
            {
                clearErrno();
            }
#elif _WIN32
            if(errornum == WSAEWOULDBLOCK || errornum == WSAEINPROGRESS ||
                 errornum == WSATRY_AGAIN)
            {
                clearErrno();
            }
#endif
        }
    }

    return result;
}

Socket::ReceiveResult Socket::receive(std::vector<uint8_t> &receiveData)
{
    ReceiveResult result = ReceiveResult::OK;
    int recv_res = -1;

    uint8_t *rec_data = new uint8_t[receiveBufferSize()];

    if(isListener())
    {
        result = ReceiveResult::ERROR_IS_LISTENER;
        goto receive_end;
    }

    if(!isConnected())
    {
        result = ReceiveResult::ERROR_NO_CONNECTION;
        goto receive_end;
    }


    recv_res = 
            recv(m_socket_fd, 
                     (char*)rec_data, 
                     receiveBufferSize(), 0);
#ifdef __linux__
    if(recv_res < 0)
    {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            result = ReceiveResult::ERROR_NOTHING_RECEIVED;
        }
        else
        {
            result = ReceiveResult::ERROR_SYSTEM;
        }
    }
#elif _WIN32
    
    if(recv_res == SOCKET_ERROR)
    {
        int last_err = WSAGetLastError();
        if(last_err == WSAEINPROGRESS ||
             last_err == WSAEWOULDBLOCK || 
             last_err == WSATRY_AGAIN)
        {
            result = ReceiveResult::ERROR_NOTHING_RECEIVED;
        }
        else
        {
            result = ReceiveResult::ERROR_SYSTEM;
        }
    }
#endif
    else if (recv_res == 0)
    {
        result = ReceiveResult::ERROR_PEER_DISCONNECTED;
    }
    else
    {
        receiveData.clear();
        receiveData = std::vector<uint8_t>(rec_data, rec_data + recv_res);
    }
    

receive_end:

    if(result == ReceiveResult::ERROR_SYSTEM)
    {
        this->close();
        setErrno();
    }
    else
    {
        clearErrno();
    }

    delete[] rec_data;

    return result;
}

size_t Socket::send(const std::vector<uint8_t> &dataToSend)
{
    size_t result = 0;

    if(isConnected() && !isListener())
    {
#ifdef __linux__
        ssize_t bytes_sent = 
        ::send(m_socket_fd, dataToSend.data(), 
                     dataToSend.size(),MSG_NOSIGNAL);
#elif _WIN32
        int bytes_sent = 
        ::send(m_socket_fd, (const char*)dataToSend.data(),
                     dataToSend.size(), 0);
#endif

        if(bytes_sent >= 0)
        {
            result = static_cast<size_t>(bytes_sent);
            clearErrno();
        }
        else
        {
            setErrno();
        }
    }

    return result;
}

int32_t Socket::socketNumber()
{
    return m_socket_fd;
}

bool Socket::isListener()
{
    return m_is_listener;
}

uint16_t Socket::listenPort()
{
    return m_listen_port;
}


bool Socket::isConnected()
{
    return m_is_connected;
}


bool Socket::isListening()
{
    return m_is_listening;
}

std::string Socket::peerIpAddress()
{
    char *addr = nullptr;

    if(!isListener() && isConnected())
    {
        sockaddr *p_addr_info_storage = nullptr;
#ifdef __linux__
        sockaddr addr_info_storage;
        p_addr_info_storage = &addr_info_storage;
        unsigned length = sizeof(sockaddr);
        if(getpeername(m_socket_fd, p_addr_info_storage,
             static_cast<socklen_t*>(&length)) == 0)
#elif _WIN32
        //On "Windows "getpeername()" fails when using a "sockaddr" as storage for the
        //IP address (like it is used on Linux). That's why I decided to just use an arbitrary array of chars which
        //is much larger than a "sockaddr" struct.
        char addr_info_storage[128];
        std::memset(&addr_info_storage, 0, sizeof(addr_info_storage));
        p_addr_info_storage = reinterpret_cast<sockaddr*>(addr_info_storage);
        int length = sizeof(addr_info_storage);


        if(getpeername(static_cast<SOCKET>(m_socket_fd), p_addr_info_storage,
             &length) != SOCKET_ERROR)
#endif
        {
            if(p_addr_info_storage->sa_family == AF_INET)
            {
                sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in*>(p_addr_info_storage);
                addr = new char[INET_ADDRSTRLEN + 1];
                inet_ntop(AF_INET, &(ipv4->sin_addr),
                                    addr, INET_ADDRSTRLEN);
            }
            else
            {
                sockaddr_in6 *ipv6 = reinterpret_cast<sockaddr_in6*>(p_addr_info_storage);
                addr = new char[INET6_ADDRSTRLEN + 1];
                inet_ntop(AF_INET6, &(ipv6->sin6_addr),
                                    addr, INET6_ADDRSTRLEN);
            }
        }
    }

    std::string addr_str;

    if(addr)
    {
        addr_str = std::string(addr);
		delete[] addr;
    }

    return addr_str;
}


uint16_t Socket::peerPort()
{
    uint16_t portnum = 0;

    if(!isListener() && isConnected())
    {
        sockaddr* p_addr_info_storage = nullptr;
#ifdef __linux__
        sockaddr addr_info_storage;
        p_addr_info_storage = &addr_info_storage;
        unsigned length = sizeof(sockaddr);
        if(getpeername(m_socket_fd, p_addr_info_storage,
             static_cast<socklen_t*>(&length)) == 0)
#elif _WIN32
        //On "Windows "getpeername()" fails when using a "sockaddr" as storage for the
        //IP address (like it is used on Linux). That's why I decided to just use an arbitrary array of chars which
        //is much larger than a "sockaddr" struct.
        char addr_info_storage[128];
        std::memset(&addr_info_storage, 0, sizeof(addr_info_storage));
        p_addr_info_storage = reinterpret_cast<sockaddr*>(addr_info_storage);
		int length = sizeof(addr_info_storage);
        if(getpeername(static_cast<SOCKET>(m_socket_fd), p_addr_info_storage,
             &length) != SOCKET_ERROR)
#endif
        {
            if(p_addr_info_storage->sa_family == AF_INET)
            {
                sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in*>(p_addr_info_storage);
                portnum = ntohs(ipv4->sin_port);
            }
            else if(p_addr_info_storage->sa_family == AF_INET6)
            {
                sockaddr_in6 *ipv6 = reinterpret_cast<sockaddr_in6*>(p_addr_info_storage);
                portnum = ntohs(ipv6->sin6_port);
            }
        }
    }

    return portnum;
}

std::string Socket::ownIpAddress()
{
    std::string addr;
    constexpr static size_t len = 100;

    // Allocate some space for the
    // IP address string.
    // Guessing that 100 bytes should be more
    // than enough.
    char *c_string = new char[len];

    if(gethostname(c_string, len) != -1)
    {
        addr = std::string(c_string);
    }

    delete[] c_string;

    return addr;
}

std::string Socket::peerName()
{
    char *host_name = nullptr;
    int name_info_result = 0;

    if(!isListener() && isConnected())
    {
        sockaddr* p_addr_info_storage = nullptr;
#ifdef __linux__
        sockaddr addr_info_storage;
        p_addr_info_storage = &addr_info_storage;
        unsigned length = sizeof(sockaddr);
        if(getpeername(m_socket_fd, p_addr_info_storage,
             static_cast<socklen_t*>(&length)) == 0)
#elif _WIN32
        //On "Windows "getpeername()" fails when using a "sockaddr" as storage for the
        //IP address (like it is used on Linux). That's why I decided to just use an arbitrary array of chars which
        //is much larger than a "sockaddr" struct.
        char addr_info_storage[128];
        std::memset(&addr_info_storage, 0, sizeof(addr_info_storage));
        p_addr_info_storage = reinterpret_cast<sockaddr*>(addr_info_storage);
        int length = sizeof(addr_info_storage);
        if(getpeername(static_cast<SOCKET>(m_socket_fd), p_addr_info_storage,
             &length) == 0)
#endif
        {
            host_name = new char[NI_MAXHOST];
            name_info_result = getnameinfo(
                                            p_addr_info_storage,
                                            static_cast<socklen_t>(length),
                                            host_name,
                                            NI_MAXHOST,
                                            NULL,
                                            0,
                                            0);
        } 
    }

    std::string host_name_str;

    if(name_info_result == 0)
    {
		if(host_name)
		{
			host_name_str = std::string(host_name);
		}
    }

	if(host_name)
	{
		delete[] host_name;
	}

    return host_name_str;
}

std::string Socket::getLastErrnoString()
{
    std::string errstr;
    int lasterrno = getLastErrno();

    if(lasterrno != 0)
    {
#ifdef __linux__
        errstr = std::string(strerror(lasterrno));
#elif _WIN32
        static char msgbuf[256];
        char *begin = msgbuf;
        char *end = msgbuf + sizeof(msgbuf);
        std::fill(begin, end, 0);

        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            lasterrno,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            msgbuf,
            sizeof(msgbuf),
            NULL);

        errstr = std::string(msgbuf);
#endif
    }

    return errstr;
}

void Socket::setReceiveBufferSize(size_t newSize)
{
    m_receive_buffer_size = newSize;
}

size_t Socket::receiveBufferSize()
{
    return m_receive_buffer_size;
}

void Socket::setSleepTime(uint32_t milliseconds)
{
    m_sleep_time = milliseconds;
}

uint32_t Socket::sleepTime()
{
    return m_sleep_time;
}

int Socket::getLastErrno()
{
    int temp = m_last_errno;
    m_last_errno = 0;
    return temp;
}

void Socket::setErrno()
{
#ifdef __linux__
        m_last_errno = errno;
#elif _WIN32
        m_last_errno = WSAGetLastError();
#endif
}

void Socket::clearErrno()
{
    m_last_errno = 0;
}

#ifdef _WIN32

void Socket::winsockInit()
{
    WSAStartup(MAKEWORD(1,1), &m_wsadata);
}

void Socket::winsockDeInit()
{
    WSACleanup();
}

#endif

}
