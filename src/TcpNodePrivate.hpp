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

#ifndef SPW_TCP_NODE_PRIVATE_HPP_
#define SPW_TCP_NODE_PRIVATE_HPP_

#include <cstdint>
#include <sys/types.h>

#include <string>
#include <vector>
#include <deque>
#include <list>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include "../include/Peer.hpp"
#include "../src/PeerPrivate.hpp"
#include <functional> 
#include <unordered_map>
#include "../include/common.hpp"
#include "ISocket.hpp"

struct addrinfo;

namespace spw
{

class TcpNodePrivate
{

public:

    TcpNodePrivate(IpVersion ipv = IpVersion::ANY);
    TcpNodePrivate(const TcpNodePrivate &other) = delete;
    virtual ~TcpNodePrivate();

    void disconnectPeer(const Peer &pr);
    void disconnectAll();
    void doListen(
        uint16_t port,
        IpVersion ipv = IpVersion::ANY);
    void stopListening();
    bool isListening();
    void connectTo(
        const std::string &ipaddr,
        uint16_t port);
    void sendData(
        const Peer &pr,
        const std::vector<uint8_t> &dat);
    uint16_t listenPort();
    void setReceiveBufferSize(size_t number_of_bytes);
    size_t receiveBufferSize();
    PeerList allPeers();
    Peer latestPeer();
    int connectTimeout();
    void setConnectTimeout(int ms);
    void setSleepTime(int ms);
    void onStartedListening(std::function<void(uint16_t)> callback);
    void onStoppedListening(std::function<void()> callback);
    void onAccept(std::function<void(Peer pr)> callback);
    void onReceive(std::function<void(Peer pr, std::vector<uint8_t>bytes)> callback);
    void onDisconnect(std::function<void(Peer pr)> callback);
    void onClosedConnection(std::function<void(Peer pr)> callback);
    void onConnect(std::function<void(Peer pr)> callback);
    void onSend(std::function<void(Peer pr, size_t amount)> callback);
    void onListenError(std::function<void (Message msg)> callback);
    void onSendError(std::function<void (Message msg)> callback);
    void onConnectError(std::function<void (Message msg)> callback);
    void onFaultyConnectionClosed(std::function<void(Peer pr, Message err)> callback);
    void setListener(ISocket *ifsock);
    void setSocketInterfaceCreateFunction(std::function<ISocket*()> new_socket_func);


protected:

    /**
     * This worker function is executed
     * by connectThread. Its purpose is to
     * wait until the user puts a connect
     * request into the queue potential_peers
     * by using the connectTo() function.
     * As soon as the queue is not empty
     * it tries to connect to all the remotes
     * in the queue and calls onConnect()
     * for each if successful or 
     * callbackConnectError() if it fails.
    */
    void _connectThreadJob(); 

    /**
     * This worker function is executed
     * by sendThread. Its purpose is to
     * wait until the user puts data 
     * into the list data_to_send by using the
     * sendData() function. As soon as the
     * list is not empty it tries to send
     * all the data in the list and calls
     * onSend() for each one if successful 
     * or callbackSendError() if it fails.
    */
    void _sendThreadJob();

    /**
     * This worker function is executed
     * by listenThread. Its purpose
     * is to wait for incoming connections or
     * data. When it receives data it calls
     * the function oncallbackReceived(). When a new
     * peer connects it calls onConnect(). 
     * If an error occurrs it calls 
     * callbackListenError().
    */
    void _listenThreadJob();

    /**
     * listenThread needs to be active as soon
     * as the first peer connects or has been
     * connected to. To make sure listenThread
     * is already running when the first connection
     * is established, this function is called by
     * doListen() and connectTo(). It checks whether
     * listenThread is running. If not, it will start
     * it.
    */
    void _startListenThreadIfNotRunning();


    /**
     * If the worker function
     * _listenThreadJob() notices
     * that there are no peers to receive data from
     * and that there also is no listen_socket_fd
     * to accept connections with, it calls this
     * function to go to pause until that changes. 
     * The pause will also end if one the following is
     * true: constructor_called, m_wakeup_listen_thread or 
     * m_changing_listener.
    */
    void _pauseUntilPeersOrListenerAvailable();

    /**
     * If the worker function _connectThreadJob()
     * notices that the potential_peers dequeue is empty
     * it calls this function to pause until
     * at least one element apperas in potential_peers.
     * The pause will also end if constructor_called is true.
    */
    void _pauseUntilQueueNotEmpty();

    /**
     * If the worker function _sendThreadJob()
     * notices that the data_so_send list is empty
     * it calls this function to pause until
     * at least on element appears in data_to_send.
     * The pause will also end if constructor_called is true.
    */
    void _pauseUntilDataToSendAvailable();


    /**
     * Creates a Message with the desired
     * head and body. Error number and
     * string will be added to body if
     * the ISocket object, that caused
     * the error, is also passed to
     * this function.
     * @param[in] h Head of Message
     * @param[in] b Body of Message
     * @param[in] sock ISocket object that 
     *                                 caused error
    */
    Message _createErrorMessage(
        const std::string &h,
        const std::string &b,
        ISocket *sock = nullptr);

    /**
     * Check wether a Peer is in
     * the peers unordered map.
     * @param[in] The Peer to be checked
     * @return Is the Peer in the vector?
    */
    bool _peerExists(uint64_t peer_id);

    static ISocket* defaultNewSocket();


private:

    const int DEFAULT_TIMEOUT_MS = 3000;
    const int DEFAULT_SLEEPTIME_MS = 10;

    using Lock = std::unique_lock<std::mutex>;
    using Condition = std::condition_variable;
    using IpAndPort = std::pair<std::string, uint16_t>;
    using IpAndPortDeque = std::deque<IpAndPort>;
    using PeerSocketList = std::unordered_map<uint64_t, ISocket*>;
    using OutBuffer = std::pair<uint64_t, std::vector<uint8_t>>;
    using OutBufferList = std::list<OutBuffer>;

    ISocket *m_listener;

    std::atomic<uint16_t> m_portnumber;
    IpVersion m_ip_version;
    std::atomic<bool> m_connect_thread_running;
    std::atomic<bool> m_send_thread_running;
    std::atomic<bool> m_listen_thread_running;
    std::atomic<bool> m_listening_enabled;
    std::atomic<bool> m_destructor_called;
    std::atomic<bool> m_listener_available;
    std::atomic<bool> m_wakeup_listen_thread;
    std::atomic<bool> m_changing_listener;

    //Timeouts
    std::atomic_int m_connect_timeout;
    std::atomic_int m_sleep_time;

    IpAndPortDeque m_potential_peers;
    OutBufferList m_data_to_send;

    std::thread m_connectThread;
    std::thread m_sendThread;
    std::thread m_listenThread;

    //Callbacks
    std::function<void(Peer pr)> m_callbackNewPeerConnected;
    std::function<void(Peer pr)> m_callbackConnectedToNewPeer;
    std::function<void(Peer pr, std::vector<uint8_t> bytes)> m_callbackReceived;
    std::function<void(Peer pr, size_t amount)> m_callbackSent;
    std::function<void(Peer pr)> m_callbackPeerDisconnected;
    std::function<void(Peer pr)> m_callbackClosedConnection;
    std::function<void(uint16_t listen_port)> m_callbackStartedListening;
    std::function<void()> m_callbackStoppedListening;
    std::function<void(Message err)> m_callbackListenError;
    std::function<void(Message err)> m_callbackSendError;
    std::function<void(Message err)> m_callbackConnectError;
    std::function<void(Peer pr, Message err)> m_callbackFaultyConnectionClosed;

    //ISocket creator
    std::function<ISocket*()> m_createNewSocketFunction;

    //Mutexes and condition variables
    std::mutex m_data_access;
    std::mutex m_callback_access;
    Condition m_peers_or_listener_available;
    Condition m_queue_not_empty;
    Condition m_data_to_send_available;

    PeerList m_peers;

    static std::atomic<uint64_t> m_connection_counter;

};

}

#endif //SPW_TCP_NODE_PRIVATE_HPP_
