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
 * @brief Holds all information about a connected peer.
 * 
 * Objects of the class TcpEndpoint can have multiple
 * connections at the same time. They use a vector
 * of Peer objects to store the information of all
 * the currently connected peers.
 * 
 * Each Peer object contains:
 * - A unique id
 * - The socket file discriptor which is used
 *   to send or receive data from the peer.
 * - The ip address of the peer.
 * - The port number the peer is using.
*/
class Peer
{
public:

  Peer(int socket, const std::string &ip, uint16_t port);
  Peer(const std::string &ip, uint16_t port);
  virtual ~Peer() {}

  virtual uint64_t id() const { return unique_id; }
  virtual int socket() const { return socket_fd; }
  virtual std::string ipAddress() const { return ip_address; }
  virtual uint16_t port() const { return port_number; }


protected:

private:

  uint64_t unique_id;
  int socket_fd;
  std::string ip_address;
  uint16_t port_number;

  static uint64_t unique_id_counter;

};

}



#endif /* PEER_H_ */
