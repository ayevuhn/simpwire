/*
Copyright (c) 2017 ayevuhn

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
 * @file TcpEndpoint.h
 *
 * Contains the declaration of the class
 * TcpEndpoint.
 */

#ifndef TCPENDPOINT_H_
#define TCPENDPOINT_H_

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <thread>
#include <ctime>
#include "Peer.h"
#include "Exceptions.h"

#define DEFAULT_TIMEOUT 2000
#define DEFAULT_BACKLOG 20
#define TIMESLICE 100 //Milliseconds between connect attempts

namespace spw
{

/**
 * @class TcpEndpoint
 * @brief Simple network communication over TCP.
 *
 * With TcpEndpoint you can easily establish
 * multiple TCP connections to any hosts 
 * and also accept multiple incoming TCP connections
 * from other clients while still maintaining the
 * previously made connection. While connected to
 * one or multiple peers TcpEndpoint
 * can exchange any amount of data with them
 * for as long as desired.
 *
 * Note that currently TcpEndpoint does not
 * support asynchronous listen, connect, read
 * or write calls. 
 *
 * The goal of TcpEndpoint is to provide a
 * simple module that helps the user establish
 * basic network connections without the need
 * to fumble around with the low level UNIX socket
 * routines as it does this for the user.
 *
 */
class TcpEndpoint
{
public:

  TcpEndpoint();
  /**
   * @param[in] port: Listen port number.
   */
  TcpEndpoint(uint16_t port);
  virtual ~TcpEndpoint();

  /**
   * Enable or disable IPv6 mode.
   * @param[in] use_or_not:
   *    If true then IPv6 is used.
   */
  virtual void useIpV6(bool use_or_not)
  {
    use_ip_v6 = use_or_not;
  }

  /**
   * Specifiy on which port to listen for
   * incoming connections.
   * @param[in] port: Listen port number.
   */
  virtual void setListenPort(uint16_t port)
  {
    portnumber = port;
  }


  /**
   * Sets the amount of time TcpEndpoint
   * will wait for a connect, listen, read or
   * write call to complete.
   * @param[in] millisecs: Timeout in milliseconds
   */
  virtual void setTimeout(uint32_t millisecs)
  {
    timeout_ms = millisecs;
  }

  /**
   * With this function you can tell
   * TcpEndpoint whether you want to
   * use the timeout. If not then
   * connect, listen, read or write calls
   * can block forever
   * (that means infinite timeout).
   * @param[in] use_timeout:
   *    If true then timeout will be used
   */
  virtual void useTimeout(bool use_timeout)
  {
    timeout_used = use_timeout;
  }

  /**
   * Check whether TcpEndpoint is using IPv6.
   * @return
   *   Is using IPv6?
   */
  virtual bool isIpV6() { return use_ip_v6; }

  /**
   * See which port is currently set.
   * @return
   *   The port number.
   */
  virtual uint16_t portNumber() { return portnumber; }


  /**
   * See timeout setting.
   * @return
   *  Timeout in milliseconds.
   */
  virtual uint32_t timeout() { return timeout_ms; }

  /**
   * Check whether timeout is used
   * @return
   *    Is timeout used?
   */
  virtual bool usingTimeout() { return timeout_used; }


  /**
   * Waits for an incoming connection.
   * This function will add a new Peer object
   * to peers if successful.
   * If timeout is not used then this
   * function may block forever.
   */
  virtual void waitForConnection();

  /**
   * Tries to connect to a host.
   * This function will add a new Peer object
   * to to peers if successful.
   * If timeout is not used then this
   * function may block forever.
   */
  virtual void connectToHost(const std::string &hostip,
                             uint16_t hostport);

  /**
   * Get all peers currently connected.
   * @return Peer Vector
  */
  virtual std::vector<const Peer*> getPeers() const;

  /**
   * Check whether there are any peers connected.
   * @return Peers available?
  */
  virtual bool peersAvailable() { return !peers.empty(); }

  /**
   * Get the latest Peer that connected or that
   * was connected to (last element in vector peers).
   * @return Last elment in vector peers
  */
  virtual const Peer* getLatestPeer() const;

  /**
   * Disconnect one specific Peer.
   * If no Peer is specified the latest Peer
   * will be disconnected (last element in vector
   * peers).
   * @param[in] pr Peer to disconnect.
   */
  virtual void disconnectPeer(Peer *pr = NULL);

  /**
   * Disconnect all Peers (vector peers
   * will be emptied).
  */
  virtual void disconnectAll();

  /**
   * Sends bytes to the peer.
   * @param[in] data : The bytes that will be sent.
   * @param[in] length: The length of data.
   * @param[in] pr: The peer the data is sent to.
   * @return How many bytes were actually sent
   */
  virtual int32_t sendBytes(const char *data,
                            int32_t length,
                            const Peer *pr = NULL);
  /**
   * Simplifies sending bytes to the peer
   * by using a std::vector<char>.
   * @param[in] data: Bytes to be sent.
   * @param[in] pr: The peer the data is sent to.
   * @return How many bytes actually were sent.
   */
  virtual int32_t sendBytes(const std::vector<char> &data,
                            const Peer *pr = NULL);

  /**
   * Simplifies sending a std::string to the peer.
   * @param[in] data: String to be sent.
   * @param[in] pr: The peer the string is sent to.
   * @return How many characters were sent.
   */
  virtual int32_t sendString(const std::string &data,
                            const Peer *pr = NULL);

  /**
   * Waits for the peer to send bytes.
   * @param[out] buffer: The storage for the received bytes.
   * @param[in] length: How many bytes are to be received
   *                   (should not exceed size of buffer).
   * @param[in] pr: The peer data is received from.
   * @return How many bytes actually were received.
   */
  virtual int32_t receiveBytes(char *buffer,
                               int32_t length,
                               Peer *pr = NULL);

  /**
   * Simplifies receiving bytes from the peer by
   * using a std::vector<char>.
   * @param[out] buffer: Storage for the received bytes.
   * @param[in] length: How many bytes are to be received.
   * @param[in] pr: The peer data is received from.
   * @return How many bytes actually were received.
   */
  virtual int32_t receiveBytes(std::vector<char> &buffer,
                               int32_t length,
                               Peer *pr = NULL);
  /**
   * Simplifies receiving a string from the peer.
   * @param[out] buffer: Storage for received string.
   * @param[in] length: How long the string should be.
   * @param[in] pr: The peer the string is received from.
   * @return How many characters were received.
   */
  virtual int32_t receiveString(std::string &buffer,
                                int32_t length,
                                Peer *pr = NULL);

  /**
   * Shows IP of this machine.
   * @return The IP address
   */
  virtual std::string getOwnIpAddress();

protected:

  /**
   * Creates new local_socket_fd,
   * binds it and sets it to listen.
   * This function is used by
   * waitForConnection()
   * @return Created acceptor successfully?
   */
  virtual bool createNewAcceptor();

  /**
   * This is a helper function for quickly
   * changing between blocking and non blocking
   * modes.
   * @return Changed mode successfully?
   */
  virtual bool changeBlocking(int sockfd,
                              bool nonblocking);


  /**
   * Checks whether the provided Peer pointer does
   * actually point to a peer inside the peers
   * vector.
   */
  virtual bool peerIsInVector(const Peer *pr);


  /**
   * Helper function to get a human readable string 
   * of the peer connected to the specified socket.
   * @param[in] sockfd Socket connected to peer.
   */
  virtual std::string getPeerIpAddress(int sockfd);


  /**
   * Helper function to get the port the peer is using.
   * @param[in] addrstorage sockaddr_storage containing
   *   peer information.
   */
  virtual uint16_t getPeerPort(const sockaddr_storage &addrstorage);



private:

  bool use_ip_v6;
  bool timeout_used;
  uint32_t timeout_ms;

  uint16_t portnumber;

  /**
   * Socket that is used to
   * listen for connections.
   */
  int listen_socket_fd;


  /**
   * Contains all the peers as Peer
   * object which are currently 
   * connected to this object.
   */
  std::vector<Peer> peers;


  /**
   * For large amounts of data (several hundreds of kilobytes)
   * it is not guaranteed that send() will transmit
   * all of it at once. And when it does transmit only
   * a part of the data it is not predictable how many
   * bytes will be sent.
   *
   * This function tries to minimize that behavior by
   * calling send() several times. It still cannot
   * guarantee that all bytes will be sent at once
   * but hopefully the transmission rate will be better
   * than that of calling send() just once.
   *
   * Source:
   * http://beej.us/guide/bgnet/output/
   * html/singlepage/bgnet.html#select
   *
   * @param[in] s: Socket file descriptor.
   * @param[in] buf: Data to be sent.
   * @param[in/out] len: How many bytes should be sent.
   *                   This value is changed after execution
   *                   to show how many bytes were actually sent.
   * @param[in] blocking: Indicates whether the socket is allowed
   *                    to block.
   * @return Success?
   */
  bool sendAllPossible(int s, const char* buf,
                        int32_t *len, bool blocking);

  /**
   * Tries to receive as many bytes as possible at once
   * using the recv() function.
   * This function may still receive
   * less than the buffer size specified in len.
   *
   * @param[in] s: Socket file descriptor.
   * @param[in] buf: Data to be sent.
   * @param[in/out] len: How many bytes should be sent.
   *                   This value is changed after execution
   *                   to show how many bytes were actually sent.
   * @param[in] blocking: Indicates whether the socket is allowed
   *                    to block.
   * @return Success?
   */
  bool receiveAllPossible(int s, char* buf,
                         int32_t *len, bool blocking);

};

} /* namespace spw */

#endif /* TCPENDPOINT_H_ */
