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

/**
 * @file Peer.cpp
 * Contains implementation of class Peer.
*/

#include "../include/Peer.hpp"
#include "PeerPrivate.hpp"

namespace spw
{

Peer::Peer() : m_private(new PeerPrivate())
{

}

Peer::Peer(const Peer &other)
{
    delete m_private;
    m_private = new PeerPrivate();
    *m_private = *other.m_private;
}

Peer& Peer::operator=(const Peer &other)
{
    delete m_private;
    m_private = new PeerPrivate();
    *m_private = *other.m_private;
    return *this;
}

Peer::Peer(Peer&& other) : m_private(nullptr)
{
    m_private = other.m_private;
    other.m_private = nullptr;
}


Peer::~Peer()
{
    if(m_private)
    {
        delete m_private;
        m_private = nullptr;
    }
}

uint64_t Peer::id() const
{
    if(m_private)
    {
        return m_private->id();
    }
    else
    {
        return 0;
    }
}

std::string Peer::ipAddress() const
{
    if(m_private)
    {
        return m_private->ipAddress();
    }
    else
    {
        return "";
    }
}

uint16_t Peer::port() const
{
    if(m_private)
    {
        return m_private->port();
    }
    else
    {
        return 0;
    }
}

std::string Peer::hostName() const
{
    if(m_private)
    {
        return m_private->hostName();
    }
    else
    {
        return "";
    }
}

bool Peer::isValid() const
{
    if(m_private)
    {
        return m_private->isValid();
    }
    else
    {
        return false;
    }
}

bool Peer::operator==(const Peer &other) const
{
    if(m_private && other.m_private)
    {
        return m_private->operator==(*other.m_private);
    }
    else
    {
        return false;
    }
}

Peer::operator bool() const
{
    if(m_private)
    {
        return m_private->operator bool();
    }
    else
    {
        return false;
    }
}

}
