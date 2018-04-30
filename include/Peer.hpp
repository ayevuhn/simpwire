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

/*
 * @file Peer.h
 *
 * Contains the declaration of the class
 * Peer.
 */

#ifndef PEER_H_
#define PEER_H_

#include <cstdint>
#include <string>

namespace spw
{

/**
 * @class Peer
 * @brief Holds all information about a connected peers.
 * 
 * Objects of the class TcpNode  
 * can have multiple connections at the same time. 
 * They use a vector of Peer objects to store 
 * the information of all the currently connected peers.
 * 
 * Each Peer object contains:
 * - A connection id
 * - The socket file discriptor which is used
 *   to send or receive data from the peer.
 * - The ip address of the peer.
 * - The port number the peer is using.
 * 
 * A Peer object can convert to bool. So
 * it can be used in a conditional:
 * Peer pr;
 * if(pr)
 * { ... }
 * Peer objects evaluate to false when
 * their socket_fd is -1, 
 * their ip_address is an empty string
 * and their port number is zero.
 * That applies to all Peer constructed
 * whithout parameters. Those "null Peers"
 * are useful as return value if you
 * want to indicate that no Peers 
 * were found by a function that returns
 * Peer objects like getPeer() or similar.
*/
class Peer
{
public:

  Peer(uint64_t conn_id, int socket, const std::string &ip, uint16_t port);
  Peer();
  
  virtual ~Peer() {}

  virtual uint64_t id() const { return connection_id; }
  virtual int socket() const { return socket_fd; }
  virtual std::string ipAddress() const { return ip_address; }
  virtual uint16_t port() const { return port_number; }

  bool operator==(const Peer &other) const
  {
    return socket_fd == other.socket_fd &&
           ip_address == other.ip_address &&
           port_number == other.port_number;

  }

  explicit operator bool() const 
  {
    return socket_fd != -1 &&
           !ip_address.empty() &&
           port_number != 0;
  }


protected:

private:

  uint64_t connection_id;
  int socket_fd;
  std::string ip_address;
  uint16_t port_number;

};

}



#endif /* PEER_H_ */
