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
 * @file TcpNode.hpp
 *
 * Contains declarations for the class
 * TcpNode and the struct Message.
 * 
 */
#ifndef TCPNODE_H
#define TCPNODE_H
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
#include "Peer.hpp"

#include <functional> 
#include <unordered_map>
#include "common.hpp"

struct addrinfo;

namespace spw 
{

class TcpNodePrivate;

/**
 * @class TcpNode
 * @brief Simple asynchronous network 
 *                communaction over TCP.
 * 
 * With TcpNode you can easily establish
 * multiple TCP connections to any hosts 
 * and also accept multiple incoming TCP connections
 * from other clients while still maintaining the
 * previously made connection. While connected to
 * one or multiple peers TcpNode
 * can exchange any amount of data with them
 * for as long as desired.
 *
 * The goal of TcpNode is to provide a
 * simple module that helps the user establish
 * basic network connections without the need
 * to fumble around with the low level UNIX socket
 * routines as it does this for the user.
*/
#ifdef _WIN32
class DLL_IMPORT_EXPORT TcpNode
#else
class TcpNode 
#endif
{

public:

    /**
     * Constructor with optional IP version.
     * @ipv Describes which IP version will
     *    be used when listening.
    */
    TcpNode(IpVersion ipv = IpVersion::ANY);

    /**
      Copying is forbidden.
    */
    TcpNode(const TcpNode &other) = delete;

    virtual ~TcpNode();


    /**
     * Delete specified Peer from list.
     * Close its socket fd.
     * @params[in] pr Peer to be disconnected.
    */
    virtual void disconnectPeer(const Peer &pr);


    /**
     * Delete all Peers from list.
     * Close all peer socket fds.
    */
    virtual void disconnectAll();

    /**
     * Enable listening for remote peers. 
     * This will open a listen socket.
     * @param[in] port Tcp port to listen at.
     * @param[in] ipv Specify IPv4, IPv6 or any.
    */
    virtual void doListen(
        uint16_t port,
        IpVersion ipv = IpVersion::ANY);

    /**
     * Disable listening for remote peers.
     * Listen socket will be closed.
    */
    virtual void stopListening();
    
    /**
     * Check wether remote peers can connect to TcpNode.
     * @return Listening?
    */
    virtual bool isListening();

    /**
     * Connect this TcpNode to the specified IP and
     * port. This function will automatically
     * choose between IPv4 and IPv6.
     * @param[in] ipaddr IP address of remote peer
     * @param[in] port Port of remote peer
    */
    virtual void connectTo(const std::string &ipaddr,
                           uint16_t port);

    /**
     * Send vector of chars to specified peer.
     * @param[in] pr The receiver of your data
     * @param[in] dat The data you want to transmit
    */
    virtual void sendData(
        const Peer &pr, 
        const std::vector<uint8_t> &dat);


    /**
     * @return Listen port that was set by
     *                 doListen().
    */
    uint16_t listenPort();

    /**
     * Specified the maximum length of the character 
     * vector that you get from the onReceive() 
     * callback.
     * @param[in] number_of_bytes Desired max length
    */
    void setReceiveBufferSize(size_t number_of_bytes);

    /**
     * Shows the maximum length of the
     * character that you get from the
     * onReceive() callback.
     * @return The current maximum length
    */
    size_t receiveBufferSize();

    /**
      * Returns a list of all currently connected Peers
      * (the list is actually a std::unordered_map).
      * If no Peers are connected, the returned
      * list will be empty.
      * @return All currently connected Peers.
    */
    PeerList allPeers();

    /**
     * Returns latest Peer which
     * connected to this TcpNode
     * or was connected to.
     * It is the Peer with the
     * currently greatest connection id.
     * If there are no peers, an invalid
     * Peer will be returned.
     * @return The latest Peer
    */
    Peer latestPeer();

    /**
     * Returns current connect timeout in
     * milliseconds.
    */
    int connectTimeout();

    /**
      * Set a desired connect timeout.
      * @param[in] ms Timeout in milliseconds
    */
    void setConnectTimeout(int ms);

    /**
      * To reduce CPU load, TcpNode sleeps for a few
      * milliseconds after each loop in its background threads.
      * The default sleep time is 10 milliseconds.
      * Use this function to set your
      * preferred sleep time
      * @param[in] ms Sleep time in milliseconds.
    */
    void setSleepTime(int ms);

    /**
      * Specifies which function is called
      * when TcpNode starts listening.
      * @param[in] cb Started listening callback function
    */
    void onStartedListening(
        std::function<void(uint16_t)> callback);

    /**
      * Specifies which function is called when
      * TcpNode stops listening.
      * @param[in] cb Stopped listening callback function.
    */
    void onStoppedListening(
        std::function<void()> callback);

    /**
     * Specifies which function
     * should be called when a remote
     * peer connects to this TcpNode.
     * @param[in] cb Accept callback function
    */
    void onAccept(
        std::function<void(Peer pr)> callback);

    /**
     * Specifies which function is called
     * when this TcpNode receives data
     * from a remote Peer.
     * @param[in] cb Receive callback function
    */
    void onReceive(
        std::function<void(Peer pr, 
                std::vector<uint8_t> bytes)> callback);

    /**
     * Specifies which function is called
     * when a remote peer disconnects from
     * this TcpNode.
     * @param[in] cb Disconnect callback function
    */
    void onDisconnect(
        std::function<void(Peer pr)> callback);


    /**
      * Specifies which function is called
      * when a connection to a remote peer
      * is closed by TcpNode.
      * @param[in] cb Connection closed callback function
    */
    void onClosedConnection(
        std::function<void(Peer pr)> cb);

    /**
     * Specifies which function is called
     * when this TcpNode successfully 
     * established a connection to a remote
     * peer.
     * @param[in] cb Connect callback function
    */
    void onConnect(
        std::function<void(Peer pr)> callback);

    /**
     * Specifies which funciton is called
     * when this TcpNode successfully 
     * sent data to remote a peer.
     * @param[in] callback Send callback function
    */
    void onSend(
        std::function<void(Peer pr, size_t amount)> callback);

    /**
     * Specifies which function is called
     * when an error occurrs while TcpNode
     * is accepting connections or waiting
     * for data.
     * @param[in] callback Connect callback function
    */
    void onListenError(
        std::function<void (Message msg)> callback);

    /**
    * Specifies which function is called
    * when an error occurrs while TcpNode
    * is accepting connections or waiting
    * for data.
    * @param[in] callback Connect callback function
    */
    void onSendError(
        std::function<void (Message msg)> callback);
    /**
    * Specifies which function is called
    * when an error occurrs while TcpNode
    * is trying to connect to a remote Peer.
    * @param[in] callback Connect callback function
    */
    void onConnectError(
        std::function<void (Message msg)> callback);

    /**
      * Specifies which function is called
      * when a remote peer is disconnected due
      * to some sudden error.
      * @param[in] callback Faulty connection callback function.
    */
    void onFaultyConnectionClosed(
        std::function<void(Peer pr, Message err)> callback);

private:
    TcpNodePrivate *m_private = nullptr;

};

}

#endif
