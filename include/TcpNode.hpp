/*
Copyright (c) 2017 Ivan Brebric

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
#include <sys/socket.h>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include "Peer.hpp"

namespace spw 
{

/**
 * This is used as the timeout when
 * listening, connecting, sending and
 * receiving.
*/
constexpr int TIMEOUT_MS = 1000;

/**
 * Specify which IP version should be
 * used when listening. Note: When
 * using ANY, the IP version will
 * be chosen automatically. Usually
 * IPv4 will be chosen.  
*/
enum class IpVersion {ANY,IPv4,IPv6};

/**
 * @struct Message
 * Message objects are used by TcpNode
 * to deliver error messages.
*/
struct Message
{
  std::string head;
  std::string body;
};


/**
 * @class TcpNode
 * @brief Simple asynchronous network 
 *        communaction over TCP.
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
class TcpNode 
{

public:

  /**
   * Constructor with optional IP version.
   * @ipv Describes which IP version will
   *  be used when listening.
  */
  TcpNode(IpVersion ipv = IpVersion::ANY);

  TcpNode(const TcpNode &other) = delete;

  virtual ~TcpNode();

  /**
   * Delete specified Peer from vector.
   * Close its socket fd.
   * @params[in] pr Peer to be disconnected.
  */
  virtual void disconnectPeer(Peer pr);

  /**
   * Delete Peer specified by its ip address
   * and port from vector.
   * Close its socket fd.
   * @params[in] ip IP address of Peer to 
   *                be disconnected.
   * @params[in] port Port of Peer to be
   *             disconnected.
  */
  virtual void disconnectPeer(
       const std::string &ip,
       uint16_t port);

  /**
   * Delete Peer specified by its ID.
   * Close its socket fd.
   * @params[in] peerId The id of the peer
   *                    to be disconnected.
  */
  virtual void disconnectPeer(uint64_t peerId);

  /**
   * Delete all Peers from vector.
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
  virtual bool isListening()
  {
    return listener_available;
  }

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
  virtual void sendData(Peer pr, const std::vector<char> &dat);

  /**
   * Send vector of char to peer specified
   * by ip address and port.
   * @param[in] ip Ip address of peer
   * @param[in] port Port of peer
   * @param[in] dat Data to send 
  */
  virtual void sendData(
         const std::string &ip,
         uint16_t port,
         const std::vector<char> &dat);

  /**
   * Send character vector to the peer
   * whose ID matches peerId.
   * @param[in] peerId Id of targetPeer
   * @param[in] dat Data to send
  */
  virtual void sendData(
    uint64_t peerId,
    const std::vector<char> &dat);

  /**
   * @return Listen port that was set by
   *         doListen().
  */
  uint16_t listenPort() 
  {
    return portnumber;
  }

  /**
   * Specified the maximum length of the character 
   * vector that you get from the onReceive() 
   * callback.
   * @param[in] number_of_bytes Desired max length
  */
  void setReceiveBufferSize(size_t number_of_bytes) 
  {
    receive_buffer_size = number_of_bytes;
  }

  /**
   * Shows the maximum length of the
   * character that you get from the
   * onReceive() callback.
   * @return The current maximum length
  */
  size_t receiveBufferSize() 
  {
    return receive_buffer_size;
  }

  /**
   * Return a vector of all the
   * Peer that are currently connected
   * to this TcpNode. It is empty
   * if there are no connections.
   * @return Vector of Peers
  */
  std::vector<Peer> allPeers() 
  {
    Lock lck(data_access);
    return peers;
  }

  /**
   * Returns latest Peer which
   * connected to this TcpNode
   * or was connected to.
   * It is the last elment
   * in the peers vector.
   * If there are no peers, a null
   * Peer will be returned.
   * @return The latest Peer
  */
  Peer latestPeer() 
  {
    Lock lck(data_access);
    if(!peers.empty())
      return peers.back();
    else
      return Peer();
  }

  /**
   * Specifies which function
   * should be called when a remote
   * peer connects to this TcpNode.
   * @param[in] cb Accept callback function
  */
  void onAccept(
    std::function<void(Peer pr)> cb)
  { 
    Lock lck(data_access);
    callbackNewPeerConnected = cb;
  }

  /**
   * Specifies which function is called
   * when this TcpNode receives data
   * from a remote Peer.
   * @param[in] cb Receive callback function
  */
  void onReceive(
    std::function<void(Peer pr, 
        std::vector<char> bytes)> callback)
  {
    Lock lck(data_access);
    callbackReceived = callback;
  }

  /**
   * Specifies which function is called
   * when a remote peer disconnects from
   * this TcpNode.
   * @param[in] cb Disconnect callback function
  */
  void onDisconnect(
    std::function<void(Peer pr)> callback)
  {
    Lock lck(data_access);
    callbackPeerDisconnected = callback;
  }

  /**
   * Specifies which function is called
   * when this TcpNode successfully 
   * established a connection to a remote
   * peer.
   * @param[in] cb Connect callback function
  */
  void onConnect(
    std::function<void(Peer pr)> callback)
  {
    Lock lck(data_access);
    callbackConnectedToNewPeer = callback;
  }

  /**
   * Specifies which funciton is called
   * when this TcpNode successfully 
   * sent data to remote a peer.
   * @param[in] callback Send callback function
  */
  void onSend(
    std::function<void(Peer pr, size_t amount)> callback)
  {
    Lock lck(data_access);
    callbackSent = callback;
  }

  /**
   * Specifies which function is called
   * when an error occurrs while TcpNode
   * is accepting connections or waiting
   * for data.
   * @param[in] callback Connect callback function
  */
  void onListenError(
    std::function<void (Message msg)> callback)
  {
    Lock lck(data_access);
    callbackListenError = callback;
  }

  /**
  * Specifies which function is called
  * when an error occurrs while TcpNode
  * is accepting connections or waiting
  * for data.
  * @param[in] callback Connect callback function
  */
  void onSendError(
    std::function<void (Message msg)> callback)
  {
    Lock lck(data_access);
    callbackSendError = callback;
  }

  /**
  * Specifies which function is called
  * when an error occurrs while TcpNode
  * is trying to connect to a remote Peer.
  * @param[in] callback Connect callback function
  */
  void onConnectError(
    std::function<void (Message msg)> callback)
  {
    Lock lck(data_access);
    callbackConnectError = callback;
  }
  
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
   * Adds socket file descriptors from all
   * elements in the peers vector to fd_set fds.
   * All peer socket fds are compared to each other
   * and to the listen_socket_fd. The one 
   * with the greatest value is returned.
   * @param[in] fds The fd_set you want the peer
   *                socket fds to be added to.  
  */
  int _addPeersToSetAndReturnBiggestFd(fd_set *fds);

  /**
   * If the worker function
   * _listenThreadJob() notices
   * that there are no peers to receive data from
   * and that there also is no listen_socket_fd
   * to accept connections with, it calls this
   * function to go to pause until that changes. 
   * The pause will also end if one the following is
   * true: constructor_called, wakeup_listen_thread or 
   * changing_listener.
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
   * Searches peers vector for a Peer whose
   * socket file descriptor is equal to sockfd
   * and returns it. If no such Peer is found
   * a default constructed Peer (a null Peer) 
   * will be returned.
   * @param[in] sockfd The socket file descriptor
  */
  Peer _getPeerBySocketFd(int sockfd);

  /**
   * Adds a new Peer to the peers vector.
   * @param[in] conn_id The connection id of the new Peer
   * @param[in] socketfd The socket fd of the new Peer
   * @param[in] ip The IP address of the new Peer
   * @param[in] p The port of the new Peer.
  */
  void _addNewPeer(
      uint64_t conn_id, int socketfd, const std::string &ip, uint16_t p);

  /**
   * Creates a Message with the desired
   * head and body. If errno is not zero
   * it will also be added to the Message body
   * along with strerror.
   * @param[in] h Head of Message
   * @param[in] b Body of Message
  */
  Message _createErrorMessage(
    const std::string &h,
    const std::string &b);

  /**
   * Check wether a Peer is in
   * the peers vector.
   * @param[in] The Peer to be checked
   * @return Is the Peer in the vector?
  */
  bool _peerInVector(const Peer &pr);

  /**
   * Search peer whose ip address
   * and port match the provided
   * ip address and port. If no match
   * is found, a null Peer will be 
   * returned
   * @param[in] ip Ip address of Peer
   * @param[in] port Port of Peer
  */
  Peer _findPeerByIpAndPort(
      const std::string &ip,
      uint16_t port);

  /**
   * Search peer whose id matches
   * the provided id. If no match is found,
   * a null Peer will be returned.
   * @param[in] peerId Id of wanted peer.
   */
  Peer _findPeerById(uint64_t peerId);

  /**
   * Used by the worker function 
   * _listenThreadJob() to
   * poll the read and accept file
   * descriptors.
   * @param[in] greatest_fd File descriptor with
   *                        greatest value
   * @param[in] read_file_descriptors
   *            A set of file descriptors to
   *            read from
   * @return Amount of read file descriptor
   *         changes. In case of an error -1 is
   *         returned.
  */
  int _applySelectToReadFds( 
    int greatest_fd, 
    fd_set *read_file_descriptors);

  /**
   * Used by the worker function
   * _listenThreadJob().
   * It checks all the socket fds and
   * the listen_socket_fd for callbackReceived
   * data or incoming connections from
   * remote peers. If data is callbackReceived
   * or a new peer has connected it
   * calls the corresponding callback
   * (callbackNewPeerConnected() or 
   * callbackReceived() ).
   * @params[in] greatest_file_descriptors
   * @params[in] changed_file_descriptors
   *             The fd_set altered by select()
   * @params[in] master_fd_set
   *             The fd_set before it was passed
   *             to select()
  */
  void _takeCareOfAllChangedReadFds(
            int greatest_file_descriptor,
            fd_set *changed_file_descriptors,
            fd_set *master_fd_set);

  /**
   * Used by the worker function 
   * _sendThreadJob() to 
   * poll for changes in write 
   * socket fds.
   * @params[in] peer_socket_fd
   *             Socket fd to poll 
   *             for changes
   * @params[in] write_set
   *             The fd_set 
   *             peer_socket_fd is in.
  */
  int _applySelectToWriteFds(
        int peer_socket_fd,
        fd_set *write_set);

   /**
   * Creates new socket fd,
   * binds it and sets it to listen.
   * @return The created fd. It is -1 on failure.
   */
  int _createNewListener(
      IpVersion ipversion,
      uint16_t portnum);

  /**
   * This is a helper function for quickly
   * changing between blocking and non blocking
   * modes.
   * @param[in] sockfd Socket fd to be changed
   * @param[in] nonblocking Nonblocking or blocking
   * @return Changed mode successfully?
   */
  bool _changeBlocking(int sockfd,bool nonblocking);

  /**
   * Shows IP of this machine.
   * @return The IP address
   */
  std::string _getOwnIpAddress();

  /**
   * Helper function to get a human readable string 
   * of the remote peer connected to the specified socket.
   * @param[in] sockfd Socket connected to peer.
   */
  std::string _getRemoteIpAddress(int sockfd);


  /**
   * Helper function to get the port the remote peer is using.
   * @param[in] addrstorage sockaddr_storage containing
   *   peer information.
   * @return The port number
   */
  uint16_t _getRemotePort(const sockaddr_storage &addrstorage);

  /**
  * Tries to receive as many bytes as possible at once
  * using the recv() function.
  * This function may still receive
  * less than the buffer size specified in len.
  *
  * @param[in] s: Socket file descriptor.
  * @param[in] buf: Data to be callbackSent.
  * @param[in/out] len: How many bytes should be callbackSent.
  *                   This value is changed after execution
  *                   to show how many bytes were actually callbackSent.
  * @param[in] blocking: Indicates whether the socket is allowed
  *                    to block.
  * @return Success?
  */
  bool _receiveAllPossible(int s, char* buf,
      int32_t *len, bool blocking);


  /**
 * For large amounts of data (several hundreds of kilobytes)
 * it is not guaranteed that send() will transmit
 * all of it at once. And when it does transmit only
 * a part of the data it is not predictable how many
 * bytes will be callbackSent.
 *
 * This function tries to minimize that behavior by
 * calling send() several times. It still cannot
 * guarantee that all bytes will be callbackSent at once
 * but hopefully the transmission rate will be better
 * than that of calling send() just once.
 *
 * Source:
 * http://beej.us/guide/bgnet/output/
 * html/singlepage/bgnet.html#select
 *
 * @param[in] s: Socket file descriptor.
 * @param[in] buf: Data to be callbackSent.
 * @param[in/out] len: How many bytes should be callbackSent.
 *                   This value is changed after execution
 *                   to show how many bytes were actually callbackSent.
 * @param[in] blocking: Indicates whether the socket is allowed
 *                    to block.
 * @return Success?
 */
  bool _sendAllPossible(int s, const char* buf,
    int32_t *len, bool blocking);


private:

  typedef std::unique_lock<std::mutex> Lock;
  typedef std::condition_variable Condition;
  typedef std::pair<std::string, uint16_t> IpAndPort;
  typedef std::pair<int, std::vector<char>> SocketOfOutBuffer;
  typedef std::deque<IpAndPort> IpAndPortDeque;
  typedef std::list<SocketOfOutBuffer> SocketOfOutBufferList;

 
  std::atomic<int> listen_socket_fd;
  std::atomic<uint16_t> portnumber;
  IpVersion ip_version;
  std::atomic<bool> connect_thread_running;
  std::atomic<bool> send_thread_running;
  std::atomic<bool> listen_thread_running;
  std::atomic<bool> listener_requested;
  std::atomic<size_t> receive_buffer_size;
  std::atomic<bool> destructor_called;
  std::atomic<bool> listener_available;
  std::atomic<bool> wakeup_listen_thread;
  std::atomic<bool> changing_listener;

  IpAndPortDeque potential_peers;
  SocketOfOutBufferList data_to_send;

  std::thread connectThread;
  std::thread sendThread;
  std::thread listenThread;

  //Callbacks
  std::function<void(Peer pr)> callbackNewPeerConnected;
  std::function<void(Peer pr)> callbackConnectedToNewPeer;
  std::function<void(Peer pr, std::vector<char> bytes)> callbackReceived;
  std::function<void(Peer pr, size_t amount)> callbackSent;
  std::function<void(Peer pr)> callbackPeerDisconnected;
  std::function<void(Message err)> callbackListenError;
  std::function<void(Message err)> callbackSendError;
  std::function<void(Message err)> callbackConnectError;

  //Mutex and condition variables
  std::mutex data_access;
  Condition peers_or_listener_available;
  Condition queue_not_empty;
  Condition data_to_send_available;

  std::vector<Peer> peers;

  static uint64_t connection_counter;

};

}

#endif