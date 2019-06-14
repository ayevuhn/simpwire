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

#include "TcpNodePrivate.hpp"
#include "Socket.hpp"
#include "../include/Peer.hpp"
#include "PeerPrivate.hpp"

namespace spw
{

std::atomic<uint64_t> TcpNodePrivate::m_connection_counter(0);

TcpNodePrivate::TcpNodePrivate(IpVersion ipv)  :
    m_portnumber(0),
    m_ip_version(ipv),
    m_connect_thread_running(false),
    m_send_thread_running(false),
    m_listen_thread_running(false),
    m_listening_enabled(false),
    m_destructor_called(false),
    m_listener_available(false),
    m_wakeup_listen_thread(false),
    m_changing_listener(false),
    m_connect_timeout(DEFAULT_TIMEOUT_MS),
    m_sleep_time(DEFAULT_SLEEPTIME_MS),
    m_callbackNewPeerConnected(nullptr),
    m_callbackConnectedToNewPeer(nullptr),
    m_callbackReceived(nullptr),
    m_callbackSent(nullptr),
    m_callbackPeerDisconnected(nullptr),
    m_callbackClosedConnection(nullptr),
    m_callbackStartedListening(nullptr),
    m_callbackStoppedListening(nullptr),
    m_callbackListenError(nullptr),
    m_callbackSendError(nullptr),
    m_callbackConnectError(nullptr),
    m_createNewSocketFunction(nullptr)
{
    m_createNewSocketFunction = defaultNewSocket;
    m_listener = m_createNewSocketFunction();
}

TcpNodePrivate::~TcpNodePrivate()
{
    m_destructor_called = true;
    try
    {
        if(m_connect_thread_running)
        {
            m_connect_thread_running = false;
            m_queue_not_empty.notify_one();
            if(m_connectThread.joinable())
            {
                m_connectThread.join();
            }
        }
        
        if(m_send_thread_running)
        {
            m_send_thread_running = false;    
            m_data_to_send_available.notify_one();
            if(m_sendThread.joinable())
            {
                m_sendThread.join();
            }
        } 

        if(m_listen_thread_running)
        {
            m_listen_thread_running = false;
            m_peers_or_listener_available.notify_one();
            if(m_listenThread.joinable())
            {
                m_listenThread.join();
            }
        }
    }
    catch (const std::system_error &e)
    {
        //Ignore exception if it appears
    }

    disconnectAll();
    m_listener->close();
    delete m_listener;
}

void TcpNodePrivate::doListen(uint16_t port, IpVersion ipv)
{
    Lock lck(m_data_access);
    m_portnumber = port;
    m_listening_enabled = true;
    if(m_listener_available) m_changing_listener = true;

    m_ip_version = ipv;
    m_wakeup_listen_thread = true;
    lck.unlock();
    m_peers_or_listener_available.notify_one();

    _startListenThreadIfNotRunning();
}

bool TcpNodePrivate::isListening()
{
    return m_listening_enabled;
}

uint16_t TcpNodePrivate::listenPort()
{
    return m_portnumber;
}

PeerList TcpNodePrivate::allPeers()
{
    Lock lck(m_data_access);
    return m_peers;
}

void TcpNodePrivate::onStartedListening(std::function<void(uint16_t)> callback)
{
    Lock lck(m_callback_access);
    m_callbackStartedListening = callback;
}

void TcpNodePrivate::onStoppedListening(std::function<void()> callback)
{
    Lock lck(m_callback_access);
    m_callbackStoppedListening = callback;
}

void TcpNodePrivate::onAccept(std::function<void(Peer pr)> callback)
{
    Lock lck(m_callback_access);
    m_callbackNewPeerConnected = callback;
}

void TcpNodePrivate::onReceive(
    std::function<void(Peer pr, std::vector<uint8_t> bytes)> callback)
{
    Lock lck(m_callback_access);
    m_callbackReceived = callback;
}

void TcpNodePrivate::onDisconnect(std::function<void(Peer pr)> callback)
{
    Lock lck(m_callback_access);
    m_callbackPeerDisconnected = callback;
}

void TcpNodePrivate::onClosedConnection(std::function<void(Peer pr)> callback)
{
    Lock lck(m_callback_access);
    m_callbackClosedConnection = callback;
}

void TcpNodePrivate::onConnect(std::function<void(Peer pr)> callback)
{
    Lock lck(m_callback_access);
    m_callbackConnectedToNewPeer = callback;
}

void TcpNodePrivate::onSend(std::function<void(Peer pr, size_t amount)> callback)
{
    Lock lck(m_callback_access);
    m_callbackSent = callback;
}

void TcpNodePrivate::onListenError(std::function<void (Message msg)> callback)
{
    Lock lck(m_callback_access);
    m_callbackListenError = callback;
}

void TcpNodePrivate::onSendError(std::function<void (Message msg)> callback)
{
    Lock lck(m_callback_access);
    m_callbackSendError = callback;
}

void TcpNodePrivate::onConnectError(std::function<void (Message msg)> callback)
{
    Lock lck(m_callback_access);
    m_callbackConnectError = callback;
}

void TcpNodePrivate::onFaultyConnectionClosed(
    std::function<void(Peer pr, Message err)> callback)
{
    Lock lck(m_callback_access);
    m_callbackFaultyConnectionClosed = callback;
}

void TcpNodePrivate::stopListening()
{
    if(m_listen_thread_running)
    {
        m_listening_enabled = false;
        m_wakeup_listen_thread = true;
        m_peers_or_listener_available.notify_one();
    }
}

void TcpNodePrivate::connectTo(const std::string &ipaddr,
                                                uint16_t port)
{
    Lock lck(m_data_access);
    if(!m_connect_thread_running)
    {
        m_connect_thread_running = true;
        m_connectThread = std::thread(&TcpNodePrivate::_connectThreadJob, this);
    }
    m_potential_peers.push_back(IpAndPort{ipaddr, port});
    lck.unlock();
    m_queue_not_empty.notify_one();

    _startListenThreadIfNotRunning();
}

void TcpNodePrivate::sendData(const Peer &pr, const std::vector<uint8_t> &dat)
{
    Lock lck(m_data_access);

    if(_peerExists(pr.id()))
    {
        if(!m_send_thread_running) 
        {
            m_send_thread_running = true;
            m_sendThread = std::thread(
                &TcpNodePrivate::_sendThreadJob,this);
        }
            
        m_data_to_send.push_back({pr.id(), dat});
        lck.unlock();
        m_data_to_send_available.notify_one();
    }
    else
    {
        Lock lck(m_callback_access);
        if(m_callbackSendError) m_callbackSendError(
            _createErrorMessage(
                "Send Error", "Cannot send. Not connected to" + pr.ipAddress() + 
                ":" + std::to_string(pr.port()) + "."));
    }
}

void TcpNodePrivate::_listenThreadJob()
{
    while(m_listen_thread_running)
    {
        //Enable or disable listening as requested
        if(m_listening_enabled && !m_listener_available
            || m_changing_listener)
        {
            Lock lck(m_data_access);

            if(!m_listener->listen(m_portnumber, m_ip_version))
            {
                Lock lck(m_callback_access);
                if(m_callbackListenError)
                {
                    Message errmsg =    _createErrorMessage(
                            "Listen Error", 
                            "Failed to create listener",
                            m_listener);
                    
                    lck.unlock();
                    m_callbackListenError(errmsg);
                    lck.lock();
                }

                m_listener_available = false;
                m_listening_enabled = false;
            }
            else
            {
                m_listener_available = true;
                
                Lock lck(m_callback_access);
                if(m_callbackStartedListening)
                { 
                    uint16_t portnum = m_listener->listenPort();
                    lck.unlock();
                    m_callbackStartedListening(portnum);
                    lck.lock();
                }
            }

            m_changing_listener = false;
        }
        else if(!m_listening_enabled && m_listener_available)
        {
            Lock lck(m_data_access);
            m_listener->close();
            m_listener_available = false;

            Lock lck2(m_callback_access);
            if(m_callbackStoppedListening)
            {
                lck.unlock();
                m_callbackStoppedListening();
                lck.lock();
            }
        }

        m_wakeup_listen_thread = false;

        _pauseUntilPeersOrListenerAvailable();

        if(m_destructor_called)
        {
            break;
        }

        Lock lck(m_data_access);

        //Listen for new connections if listening
        //is enabled
        if(m_listener->isListener() && 
             m_listener->isListening() &&
             m_listener_available)
        {
            ISocket *new_peer = m_listener->accept();

            if(new_peer)
            {
                uint64_t current_count = ++m_connection_counter;
                Peer np;
                np.m_private->set(
                    current_count,
                    new_peer->peerIpAddress(),
                    new_peer->peerPort(),
                    new_peer->peerName());
                np.m_private->setSocket(new_peer);
                np.m_private->setValid(true);
                m_peers.insert({current_count, np});
                
                Lock lck(m_callback_access);
                if(m_callbackNewPeerConnected)
                { 
                    lck.unlock();
                    m_callbackNewPeerConnected(np);
                    lck.lock();
                }
            }
            else if(m_listener->getLastErrno() != 0)
            {
                Lock lck(m_callback_access);
                if(m_callbackListenError)
                {
                    Message errmsg = _createErrorMessage(
                        "Listen Error", "Failed to accept", m_listener);
                    lck.unlock();
                    m_callbackListenError(errmsg);
                    lck.lock();
                }
            }
        }
         
        //Listen for incoming data from active connections
        for(auto &s : m_peers)
        {
            if(s.second.m_private->toBeDeleted())
            {
                continue;
            }

            std::vector<uint8_t> recdata;
            ISocket::ReceiveResult recres = 
                    ISocket::ReceiveResult::ERROR_NO_CONNECTION;
            
            ISocket *psock = s.second.m_private->getSocket();
            if(psock)
            {
                if(psock->isConnected())
                {
                    recres = psock->receive(recdata);
                }
            }
            
            Lock lck(m_callback_access);

            switch(recres)
            {
                case ISocket::ReceiveResult::OK:
                {
                    if(m_callbackReceived)
                    {
                        lck.unlock();
                        m_callbackReceived(s.second, recdata);
                        lck.lock();
                    }
                    break;
                }
                case ISocket::ReceiveResult::ERROR_NO_CONNECTION:
                {
                    s.second.m_private->scheduleDelete(DisconnectType::PEER_WAS_DISCONNECTED_DUE_TO_ERROR);
                    s.second.m_private->setErrorMessage(
                        _createErrorMessage("Receive Error", "Socket is not connected."));
                    break;
                }
                case ISocket::ReceiveResult::ERROR_IS_LISTENER:
                {
                    s.second.m_private->scheduleDelete(DisconnectType::PEER_WAS_DISCONNECTED_DUE_TO_ERROR);
                    s.second.m_private->setErrorMessage(
                        _createErrorMessage("Receive Error", "Socket is a listener."));
                    break;
                }
                case ISocket::ReceiveResult::ERROR_PEER_DISCONNECTED:
                {
                    s.second.m_private->scheduleDelete(DisconnectType::PEER_DISCONNECTED_THEMSELF);
                    break;
                }
                case ISocket::ReceiveResult::ERROR_SYSTEM:
                {
                    s.second.m_private->scheduleDelete(DisconnectType::PEER_WAS_DISCONNECTED_DUE_TO_ERROR);
                    s.second.m_private->setErrorMessage(_createErrorMessage(
                        "Receive Error", "Failed to receive.", s.second.m_private->getSocket()));
                    break;
                }
                case ISocket::ReceiveResult::ERROR_NOTHING_RECEIVED:
                {
                    //Do nothing
                    break;
                }
            }
        }

        //Delete next peer scheduled for deletion
        for(auto &s : m_peers)
        {
            if(s.second.m_private->toBeDeleted())
            {
                Peer temp = s.second;
                s.second.m_private->destroySocket();
                m_peers.erase(s.first);

                Lock lck(m_callback_access);

                if(temp.m_private->disconnectType() == 
                     DisconnectType::PEER_DISCONNECTED_THEMSELF &&
                     m_callbackPeerDisconnected)
                {
                    lck.unlock();
                    m_callbackPeerDisconnected(temp);
                    lck.lock();
                }
                else if(m_callbackClosedConnection &&
                                temp.m_private->disconnectType() == 
                                DisconnectType::PEER_WAS_DISCONNECTED)
                {
                    lck.unlock();
                    m_callbackClosedConnection(temp);
                    lck.lock();
                }
                else if(m_callbackFaultyConnectionClosed &&
                                temp.m_private->disconnectType() ==
                                DisconnectType::PEER_WAS_DISCONNECTED_DUE_TO_ERROR)
                {
                    lck.unlock();
                    m_callbackFaultyConnectionClosed(temp, temp.m_private->getErrorMessage());
                    lck.lock();
                }
                break;
            }
        }
     
        lck.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(m_sleep_time));
    }
}

void TcpNodePrivate::_startListenThreadIfNotRunning()
{
    if(!m_listen_thread_running)
    {
        m_listen_thread_running = true;
        Lock lck(m_data_access);
        m_listenThread = 
        std::thread(&TcpNodePrivate::_listenThreadJob, this);
    }
}

void TcpNodePrivate::_connectThreadJob()
{
    size_t queue_length = 0;

    while(m_connect_thread_running)
    {
        Lock lck(m_data_access);
        queue_length = m_potential_peers.size();
        lck.unlock();

        if(queue_length <= 0)
        {
            _pauseUntilQueueNotEmpty();
            continue;
        }

        for(size_t i = 0; i < queue_length; ++i)
        {
            ISocket *new_peer = m_createNewSocketFunction();
            lck.lock();
            std::string ip = m_potential_peers.front().first;
            uint16_t port = m_potential_peers.front().second;
            lck.unlock();

            bool timeup = false;
            int errornum = 0;
            bool connect_success = false;
            auto elapsed = std::chrono::duration<double>(0);

            do
            {
                connect_success = new_peer->connect(ip, port);
                errornum = new_peer->getLastErrno();
                auto begin = std::chrono::high_resolution_clock::now();
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(m_sleep_time));
                auto end = std::chrono::high_resolution_clock::now();
                elapsed += (end - begin);
                timeup = std::chrono::duration_cast<std::chrono::milliseconds>
                                 (elapsed).count() >= m_connect_timeout;
            }while(!connect_success && 
                        errornum == 0 &&
                        !timeup);

            if(!connect_success)
            {
                Lock lck(m_callback_access);
                if(m_callbackConnectError)
                {
                    m_callbackConnectError(
                        _createErrorMessage(
                                "Connect Error", 
                                "Failed to connect to " + ip + ":"
                                + std::to_string(port),
                                new_peer));
                }

                delete new_peer;
            }
            else
            {
                lck.lock();
                uint64_t current_count = ++m_connection_counter;
                Peer pr;
                pr.m_private->set(
                    current_count,
                    new_peer->peerIpAddress(),
                    new_peer->peerPort(),
                    new_peer->peerName());
                pr.m_private->setSocket(new_peer);
                pr.m_private->setValid(true);

                m_peers.insert({current_count, pr});
                lck.unlock();

                Lock lck(m_callback_access);
                if(m_callbackConnectedToNewPeer)
                {
                    m_callbackConnectedToNewPeer(pr);
                }
                m_peers_or_listener_available.notify_one();
            }
        }

        lck.lock();
        m_potential_peers.pop_front();
        lck.unlock();
    }
    
}

void TcpNodePrivate::_sendThreadJob()
{
    while(m_send_thread_running)
    {
        Lock lck(m_data_access);
        uint8_t to_send_count = m_data_to_send.size();

        if(to_send_count <= 0)
        {
            lck.unlock();
            _pauseUntilDataToSendAvailable();
            lck.lock();
        }
     
        if(m_data_to_send.size() > 0)
        {
            OutBufferList::iterator curr_out_buffer;
            curr_out_buffer = m_data_to_send.begin();
            auto itpeer = m_peers.find(curr_out_buffer->first);

            if(itpeer == m_peers.end())
            {
                Lock lck(m_callback_access);
                if(m_callbackSendError)
                {
                    lck.unlock();// Unlock for callback ///////
                    m_callbackSendError(
                    _createErrorMessage("Send Error",
                    "Specified peer does not exist."));
                    lck.lock();
                }                
            }
            else if(!itpeer->second.m_private->toBeDeleted())
            {
                ISocket *psocket = itpeer->second.m_private->getSocket();
                size_t bytes_sent = psocket->send(curr_out_buffer->second);
                if(bytes_sent > 0)
                {
                    Lock lck(m_callback_access);
                    if(m_callbackSent)
                    {
                        Peer pr = itpeer->second;
                        lck.unlock();// Unlock for callback ///////
                        m_callbackSent(pr, bytes_sent);
                        lck.lock();
                    }
                }
                else
                {
                    if(m_callbackSendError)
                    {
                        Message errmsg = _createErrorMessage(
                                            "Send Error", "Sending Failed", psocket);
                        itpeer->second.m_private->setErrorMessage(errmsg);
                        itpeer->second.m_private->scheduleDelete(
                            DisconnectType::PEER_WAS_DISCONNECTED_DUE_TO_ERROR);
                    }
                }
            }

            m_data_to_send.pop_front();
        }
        
    lck.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_SLEEPTIME_MS));

    }
}

void TcpNodePrivate::_pauseUntilPeersOrListenerAvailable()
{
    Lock lck(m_data_access);
    m_peers_or_listener_available.wait(
        lck, [this](){return !m_peers.empty() || 
        m_listener_available || m_destructor_called ||
        m_changing_listener || m_wakeup_listen_thread;});
}

void TcpNodePrivate::_pauseUntilQueueNotEmpty()
{
    Lock lck(m_data_access);
    m_queue_not_empty.wait(lck, [this](){
        return !m_potential_peers.empty() || m_destructor_called;});
}

void TcpNodePrivate::_pauseUntilDataToSendAvailable()
{
    Lock lck(m_data_access);
    m_data_to_send_available.wait(lck, [this](){
        return !m_data_to_send.empty() || m_destructor_called;});
}

void TcpNodePrivate::disconnectPeer(const Peer &pr)
{
    Lock lck(m_data_access);
    if(_peerExists(pr.id()))
    {
        m_peers.at(pr.id()).m_private->scheduleDelete(
            DisconnectType::PEER_WAS_DISCONNECTED);
    }
}

void TcpNodePrivate::disconnectAll()
{
    Lock lck(m_data_access);

    for(PeerList::iterator itpeer = m_peers.begin();
            itpeer != m_peers.end();
            ++itpeer)
    {
        itpeer->second.m_private->scheduleDelete(
            DisconnectType::PEER_WAS_DISCONNECTED);
    }
}

Message TcpNodePrivate::_createErrorMessage(
    const std::string &h,
    const std::string &b,
    ISocket *sock)
{
    Message m{h, b};
    if(sock)
    {
        int last_errno = sock->getLastErrno();
        
        if(last_errno != 0)
        {
            m.body += "\nError number " + std::to_string(last_errno) + " :"
                         + sock->getLastErrnoString();
        }
    }
    return m;
}

bool TcpNodePrivate::_peerExists(uint64_t peer_id)
{
    auto itpeer = m_peers.find(peer_id);
    bool exists = false;

    if(itpeer != m_peers.end())
    {
        if(itpeer->second.isValid())
        {
            exists = true;
        }
    }

    return exists;
}

int TcpNodePrivate::connectTimeout()
{
    return m_connect_timeout;
}

void TcpNodePrivate::setConnectTimeout(int ms)
{
    m_connect_timeout = ms;
}

void TcpNodePrivate::setSleepTime(int ms)
{
    m_sleep_time = ms;
}

size_t TcpNodePrivate::receiveBufferSize()
{
    Lock lck(m_data_access);
    size_t result = 0;
    if(m_listener)
    {
        result = m_listener->receiveBufferSize();
    }

    return result;
}


void TcpNodePrivate::setReceiveBufferSize(size_t number_of_bytes)
{
    Lock lck(m_data_access);
    if(m_listener)
    {
        m_listener->setReceiveBufferSize(number_of_bytes);
    }
}

Peer TcpNodePrivate::latestPeer()
{
    Lock lck(m_data_access);
    if(!m_peers.empty())
    {
        uint64_t max = 0;
        for(auto &elem : m_peers)
        {
            uint64_t current = elem.first;
            if(current > max)
            {
                max = current;
            }
        }

        return m_peers.at(max);
    }
    else
    {
        return Peer();
    }
}

ISocket* TcpNodePrivate::defaultNewSocket()
{
    return new Socket();
}

void TcpNodePrivate::setSocketInterfaceCreateFunction(
    std::function<ISocket*()> new_socket_func)
{
    Lock lck(m_data_access);
    if(new_socket_func)
    {
        m_createNewSocketFunction = new_socket_func;
    }

}

void TcpNodePrivate::setListener(ISocket *ifsock)
{
    if(ifsock)
    {
        Lock lck(m_data_access);
        m_listener->close();
        delete m_listener;
        m_listener = ifsock;

        if(m_listening_enabled)
        {
            m_changing_listener = true;
        }
    }
}

}