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

#include "PeerPrivate.hpp"

namespace spw
{

PeerPrivate::PeerPrivate() :
    m_connection_id(0),
    m_ip_address(""),
    m_port_number(0),
    m_hostname("")
{

}

PeerPrivate::~PeerPrivate()
{

}

uint64_t PeerPrivate::id()
{
    return m_connection_id;
}

std::string PeerPrivate::ipAddress()
{
    return m_ip_address;
}

uint16_t PeerPrivate::port()
{
    return m_port_number;
}

std::string PeerPrivate::hostName()
{
    return m_hostname;
}

bool PeerPrivate::isValid()
{
    return m_valid;
}

PeerPrivate& PeerPrivate::operator=(const PeerPrivate &other)
{
    m_connection_id = other.m_connection_id;
    m_ip_address = other.m_ip_address;
    m_port_number = other.m_port_number;
    m_hostname = other.m_hostname;
    m_valid = other.m_valid;
    m_to_be_deleted = other.m_to_be_deleted;
    m_socket = other.m_socket;
    m_disconn = other.m_disconn;
    m_errmsg = other.m_errmsg;
    return *this;
}

bool PeerPrivate::operator==(const PeerPrivate &other)
{
    return m_connection_id == other.m_connection_id &&
                    m_ip_address == other.m_ip_address &&
                    m_port_number == other.m_port_number &&
                    m_hostname == other.m_hostname;   
}

PeerPrivate::operator bool()
{
    return isValid();
}

void PeerPrivate::set(        
        uint64_t conn_id,
        const std::string &ip,
        uint16_t port,
        const std::string &hostname)
{
    m_connection_id = conn_id;
    m_ip_address = ip;
    m_port_number = port;
    m_hostname = hostname;
}

void PeerPrivate::setValid(bool valid)
{
    m_valid = valid;
}

void PeerPrivate::scheduleDelete(DisconnectType dt)
{
    m_disconn = dt;
    m_to_be_deleted = true;
}

bool PeerPrivate::toBeDeleted()
{
    return m_to_be_deleted;
}

void PeerPrivate::setSocket(ISocket *sock)
{
    m_socket = sock;
}

ISocket* PeerPrivate::getSocket()
{
    return m_socket;
}

DisconnectType PeerPrivate::disconnectType()
{
    return m_disconn;
}

void PeerPrivate::destroySocket()
{
    if(m_socket)
    {
        m_socket->close();
        delete m_socket;
        m_socket = nullptr;
    }
}

void PeerPrivate::setErrorMessage(Message err)
{
    m_errmsg = err;
}

Message PeerPrivate::getErrorMessage()
{
    return m_errmsg;
}


}
