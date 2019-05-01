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
 * @file TcpNode.cpp
 *
 * Contains implementation of class TcpNode. 
 */
#include "../include/TcpNode.hpp"
#include "TcpNodePrivate.hpp"
#include <iostream>
#include <cstring>

#ifdef __linux__
#include <unistd.h>
#endif

#include <fcntl.h>
#include <utility>
#include <ctime>
#include <functional>
#include <chrono>
#include <algorithm>
#include "Socket.hpp"
#include "PeerPrivate.hpp"

namespace spw
{


TcpNode::TcpNode(IpVersion ipv) : m_private(new TcpNodePrivate(ipv))
{
}

TcpNode::~TcpNode()
{
    if(m_private)
    {
        delete m_private;
        m_private = nullptr;
    }
}

void TcpNode::doListen(uint16_t port, IpVersion ipv)
{
    return m_private->doListen(port, ipv);
}

void TcpNode::stopListening()
{
    return m_private->stopListening();
}

bool TcpNode::isListening()
{
    return m_private->isListening();
}

void TcpNode::connectTo(
    const std::string &ipaddr,
    uint16_t port)
{
    return m_private->connectTo(ipaddr, port);
}

void TcpNode::sendData(const Peer &pr, const std::vector<uint8_t> &dat)
{
    return m_private->sendData(pr, dat);
}

uint16_t TcpNode::listenPort()
{
    return m_private->listenPort();
}

void TcpNode::disconnectPeer(const Peer &pr)
{
    return m_private->disconnectPeer(pr);
}

void TcpNode::disconnectAll()
{
    return m_private->disconnectAll();
}

int TcpNode::connectTimeout()
{
    return m_private->connectTimeout();
}

void TcpNode::setConnectTimeout(int ms)
{
    return m_private->setConnectTimeout(ms);
}

void TcpNode::setSleepTime(int ms)
{
    return m_private->setSleepTime(ms);
}

size_t TcpNode::receiveBufferSize()
{
    return m_private->receiveBufferSize();
}

void TcpNode::setReceiveBufferSize(size_t number_of_bytes)
{
    return m_private->setReceiveBufferSize(number_of_bytes);
}

Peer TcpNode::latestPeer()
{
    return m_private->latestPeer();
}

PeerList TcpNode::allPeers()
{
    return m_private->allPeers();
}

void TcpNode::onStartedListening(
    std::function<void(uint16_t)> callback)
{
    return m_private->onStartedListening(callback);
}

void TcpNode::onStoppedListening(
    std::function<void()> callback)
{
    return m_private->onStoppedListening(callback);
}

void TcpNode::onAccept(
    std::function<void(Peer pr)> callback)
{
    return m_private->onAccept(callback);
}

void TcpNode::onReceive(
    std::function<void(Peer pr, std::vector<uint8_t> bytes)> callback)
{
    return m_private->onReceive(callback);
}

void TcpNode::onDisconnect(
    std::function<void(Peer pr)> callback)
{
    return m_private->onDisconnect(callback);
}

void TcpNode::onClosedConnection(
    std::function<void(Peer pr)> callback)
{
    return m_private->onClosedConnection(callback);
}

void TcpNode::onConnect(
    std::function<void(Peer pr)> callback)
{
    return m_private->onConnect(callback);
}

void TcpNode::onSend(
    std::function<void(Peer pr, size_t amount)> callback)
{
    return m_private->onSend(callback);
}

void TcpNode::onListenError(
    std::function<void (Message msg)> callback)
{
    return m_private->onListenError(callback);
}

void TcpNode::onSendError(
    std::function<void (Message msg)> callback)
{
    return m_private->onSendError(callback);
}

void TcpNode::onConnectError(
    std::function<void (Message msg)> callback)
{
    return m_private->onConnectError(callback);
}

void TcpNode::onFaultyConnectionClosed(
    std::function<void (Peer pr, Message err)> callback)
{
    return m_private->onFaultyConnectionClosed(callback);
}

}

