/*
 * Peer.cpp
 *
 *  Created on: Sep 24, 2017
 *      Author: ivan
 */
#include "Peer.h"

namespace spw
{
  uint64_t Peer::unique_id_counter = 0;

  Peer::Peer(int socket, const std::string &ip, uint16_t port) :
      unique_id(++unique_id_counter),socket_fd(socket),
      ip_address(ip), port_number(port)
  {

  }

  Peer::Peer(const std::string &ip, uint16_t port) :
      unique_id(++unique_id_counter), socket_fd(-1),
      ip_address(ip), port_number(port)
  {

  }

}


