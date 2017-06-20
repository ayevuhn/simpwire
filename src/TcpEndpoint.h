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
#include "Exceptions.h"

#define DEFAULT_TIMEOUT 2000
#define DEFAULT_PORT 65432
#define DEFAULT_BACKLOG 1//Only one connection allowed
#define TIMESLICE 100 //Milliseconds between connect attempts

namespace spw
{

/**
 * @class TcpEndpoint
 * @brief Simple network communication over TCP.
 *
 * With TcpEndpoint you can easily establish
 * a TCP connection to any TCP host or wait for
 * another TCP client to connect to your
 * application. Two connected TcpEndpoints
 * can then exchange any amount of data
 * for as long as they want before
 * closing the connection.
 *
 * Note that currently TcpEndpoint does not
 * support communication with several clients
 * when used as a TCP server. It is only suitable
 * for 1 to 1 communication (this might change
 * in the future).
 *
 * The goal of TcpEndpoint is to provide a
 * simple module that helps the user establish
 * a basic network connection without the need
 * to fumble around with the low level UNIX socket
 * routines as it does this for the user.
 *
 */
class TcpEndpoint
{
public:

  TcpEndpoint();
  TcpEndpoint(uint16_t port);
  TcpEndpoint(const std::string &host_or_ip,
              uint16_t port);
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
   * Sets the port number the TcpEnpoint
   * will connect to if is a client or
   * to which port it will be bound if
   * it is a server.
   * @param[in] port:
   *    Number of the port
   */
  virtual void setPortNumber(uint16_t port)
  {
    portnumber = port;
  }

  /**
   * Sets the IP adress or URL of the host
   * TcpEndpoint will connect to.
   * This is only relevant when TcpEndpoint is
   * a client.
   * @param[in] host_or_ip:
   *    Host IP address or host URL
   *
   */
  virtual void setHost(const std::string &host_or_ip)
  {
    hostname_or_ip = host_or_ip;
  }

  /**
   * Sets the amount of time TcpEndpoint
   * will wait for an operation to complete.
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
   * operations can block forever
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
   *
   *
   */
  virtual bool isIpV6() { return use_ip_v6; }

  /**
   * See which port is currently set.
   * @return
   *   The port number.
   */
  virtual uint16_t portNumber() { return portnumber; }

  /**
   * Returns the name or the IP address of the remote
   * host. Only meaningful when TcpEndpoint is a client.
   * @return
   *   Name or IP address
   */
  virtual std::string host() { return hostname_or_ip; }

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
   * Check whether TcpEndpoint is a server.
   * @return
   *   Is server?
   */
  virtual bool isServer() { return is_server; }

  /**
   * Check whether TcpEndpoint is connected
   * to peer.
   * @return
   *   Is connected?
   *
   */
  virtual bool isConnected() { return connected; }

  /**
   * Waits for an incoming connection.
   * Calling this while being connected will
   * terminate the current connection.
   * It will also make this object a server.
   * If timeout is not used then this
   * function may block forever.
   *
   */
  virtual void waitForConnection();

  /**
   * Tries to connect to a host.
   * Calling this while being connected will
   * terminate the current connection.
   * If timeout is not used then this
   * function may block forever.
   *
   */
  virtual void connectToHost();

  /**
   * Closes comm_socket_fd to
   * disconnect from peer.
   */
  virtual void disconnect();

  /**
   * Sends bytes to the peer.
   * @param[in] data : The bytes that will be sent.
   * @param[in] length: The length of data.
   * @return How many bytes were actually sent
   *
   */
  virtual int32_t sendBytes(const char *data,
                            int32_t length);
  /**
   * Simplifies sending bytes to the peer
   * by using a std::vector<char>.
   * @param[in] data: Bytes to be sent.
   * @return How many bytes actually were sent.
   *
   */
  virtual int32_t sendBytes(const std::vector<char> &data);

  /**
   * Simplifies sending a std::string to the peer.
   * @param[in] data: String to be sent.
   * @return How many characters were sent.
   */
  virtual int32_t sendString(const std::string &data);

  /**
   * Waits for the peer to send bytes.
   * @param[out] buffer: The storage for the received bytes.
   * @param[in] length: How many bytes are to be received
   *                   (should not exceed size of buffer).
   * @return How many bytes actually were received.
   */
  virtual int32_t receiveBytes(char *buffer,
                               int32_t length);

  /**
   * Simplifies receiving bytes from the peer by
   * using a std::vector<char>.
   * @param[out] buffer: Storage for the received bytes.
   * @param[in] length: How many bytes are to be received.
   * @return How many bytes actually were received.
   */
  virtual int32_t receiveBytes(std::vector<char> &buffer,
                               int32_t length);
  /**
   * Simplifies receiving a string from the peer.
   * @param[out] buffer: Storage for received string.
   * @param[in] length: How long the string should be.
   * @return How many characters were received.
   */
  virtual int32_t receiveString(std::string &buffer,
                                int32_t length);
  /**
   * Shows IP address of the connected peer.
   * @return The IP address
   */
  virtual std::string getPeerIpAddress();

  /**
   * Shows the TCP port of the connected peer.
   * @return The port number.
   */
  virtual uint16_t getPeerPort();

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


private:

  bool is_server;
  bool connected;
  bool use_ip_v6;
  bool timeout_used;
  uint32_t timeout_ms;

  std::string hostname_or_ip;
  uint16_t portnumber;

  /**
   * Socket that is used to
   * listen for connections
   * when this object is a
   * server. This socket
   * is not needed when
   * object is in client mode.
   */
  int listen_socket_fd;

  /**
   * Socket that is used for
   * communicating with a
   * peer.
   */
  int comm_socket_fd;

  /**
   * Adress of peer that is obtained
   * when the peer connects to
   * listen_socket_fd.
   * Used only when this is a server.
   */
  sockaddr_storage remote_addr;

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
