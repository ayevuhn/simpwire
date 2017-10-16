/*
 * Peer.h
 *
 *  Created on: Jun 26, 2017
 *      Author: ivan
 */

#ifndef PEER_H_
#define PEER_H_

#include <cstdint>
#include <string>

namespace spw
{

//class TcpEndpoint;

class Peer
{
public:

  Peer(int socket, const std::string &ip, uint16_t port);
  Peer(const std::string &ip, uint16_t port);
  //Peer(const Peer &other) = delete;
  virtual ~Peer() {}

  virtual uint64_t id() const { return unique_id; }
  virtual int socket() const { return socket_fd; }
  virtual std::string ipAddress() const { return ip_address; }
  virtual uint16_t port() const { return port_number; }

//  friend void TcpEndpoint::setPeerMembers(Peer &p, int *sockfd = NULL,
//                                          std::string *ipaddr = NULL,
//                                          uint16_t *peerport = NULL);


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
