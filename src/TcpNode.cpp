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
 * @file TcpNode.cpp
 *
 * Contains implementation of class TcpNode. 
 */
#include "../include/TcpNode.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <utility>
#include <ctime>
#include <functional>
#include <chrono>
#include <algorithm>

namespace spw
{

uint64_t TcpNode::connection_counter = 0;

TcpNode::TcpNode(IpVersion ipv) : 
          listen_socket_fd(-1),
          portnumber(0),
          ip_version(ipv),
          connect_thread_running(false),
          send_thread_running(false),
          listen_thread_running(false),
          listener_requested(false),
          receive_buffer_size(1024),
          destructor_called(false),
          listener_available(false),
          wakeup_listen_thread(false),
          changing_listener(false),
          callbackNewPeerConnected(NULL),
          callbackConnectedToNewPeer(NULL),
          callbackReceived(NULL),
          callbackSent(NULL)
{

}

TcpNode::~TcpNode()
{
  destructor_called = true;
  try
  {
    if(connect_thread_running)
    {
      connect_thread_running = false;
      queue_not_empty.notify_one();
      if(connectThread.joinable())
      {
        connectThread.join();
      }
    }
    
    if(send_thread_running)
    {
      send_thread_running = false;
      data_to_send_available.notify_one();
      if(sendThread.joinable())
      {
        sendThread.join();
      }
    } 

    if(listen_thread_running)
    {
      listen_thread_running = false;
      peers_or_listener_available.notify_one();
      if(listenThread.joinable())
      {
        listenThread.join();
      }
    }
  }
  catch (const std::system_error &e)
  {
    //Ignore exception if it appears
  }

  close(listen_socket_fd);
  disconnectAll();
  
}


void TcpNode::doListen(uint16_t port, IpVersion ipv)
{
  portnumber = port;
  listener_requested = true;
  if(listener_available) changing_listener = true;

  ip_version = ipv;
  wakeup_listen_thread = true;
  peers_or_listener_available.notify_one();

  _startListenThreadIfNotRunning();
}

void TcpNode::stopListening()
{
  if(listen_thread_running)
  {
    listener_requested = false;
    wakeup_listen_thread = true;
    peers_or_listener_available.notify_one();
  }
}

void TcpNode::connectTo(const std::string &ipaddr,
                        uint16_t port)
{
  Lock lck(data_access);
  if(!connect_thread_running)
  {
    connect_thread_running = true;
    connectThread = std::thread(&TcpNode::_connectThreadJob, this);
  }
  potential_peers.push_back(IpAndPort{ipaddr, port});
  lck.unlock();
  queue_not_empty.notify_one();

  _startListenThreadIfNotRunning();
}


void TcpNode::sendData(Peer pr, const std::vector<char> &dat)
{
  if(_peerInVector(pr))
  {
    if(!send_thread_running) 
    {
      send_thread_running = true;
      sendThread = std::thread(
        &TcpNode::_sendThreadJob,this);
    }
      
    Lock lck(data_access);
    data_to_send.push_back(SocketOfOutBuffer(pr.socket(),dat));
    lck.unlock();
    data_to_send_available.notify_one();
  }
  else
  {
    if(callbackSendError) callbackSendError(
      _createErrorMessage("Send Error", "Peer is not connected."));
  }
}

void TcpNode::sendData(uint64_t peerId, const std::vector<char> &dat)
{
  Peer targetPeer = _findPeerById(peerId);
  sendData(targetPeer, dat);
}

void TcpNode:: sendData(
      const std::string &ip,
      uint16_t port,
      const std::vector<char> &dat)
{
  Peer pr = _findPeerByIpAndPort(ip,port);
  if(pr)
  {
    if(!send_thread_running) 
    {
      send_thread_running = true;
      sendThread = std::thread(
        &TcpNode::_sendThreadJob,this);
    }
    
    Lock lck(data_access);
    data_to_send.push_back(SocketOfOutBuffer(pr.socket(),dat));
    lck.unlock();
    data_to_send_available.notify_one();
  }
  else
  {
    if(callbackSendError) callbackSendError(
      _createErrorMessage("Send Error", "Peer is not connected."));
  }
}


// void TcpNode::setListenPort(uint16_t p)
// { 
//   portnumber = p;
//   if(listener_available)
//     changing_listener = true;
//   wakeup_listen_thread = true;
//   peers_or_listener_available.notify_one();
// }

int TcpNode::_addPeersToSetAndReturnBiggestFd(fd_set *fds)
{
  int current_max_fd(listen_socket_fd);
  Lock lck(data_access);
  for(Peer &pr : peers)
  {
    int socketfd = pr.socket();
    if(socketfd > current_max_fd) current_max_fd = socketfd;
    if(fds)
    {
      if(!FD_ISSET(socketfd,fds))
        FD_SET(socketfd, fds);
    } 
  }
  return current_max_fd;
}


int TcpNode::_applySelectToReadFds(
    int greatest_file_descriptor,
    fd_set *read_file_descriptors)
{
  int to_be_returned = -1;
  timeval timeout_for_select;
  timeout_for_select.tv_sec = 0;
  timeout_for_select.tv_usec = TIMEOUT_MS * 1000;

  to_be_returned = select(
      greatest_file_descriptor + 1,
      read_file_descriptors,
      NULL,
      NULL,
      &timeout_for_select);

  return to_be_returned;
}

void TcpNode::_listenThreadJob()
{
  fd_set all_read_fds;
  fd_set changed_read_fds;
  int greatest_fd(-1);
  bool using_pause(true);

  while(listen_thread_running)
  {
    //Initialize fd sets
    FD_ZERO(&all_read_fds);
    FD_ZERO(&changed_read_fds);

    if(listener_requested)
    {
      if(!listener_available)
      {
        bool listener_was_created(false);
        listen_socket_fd = _createNewListener(
                    ip_version,
                    portnumber);
        listener_was_created = listen_socket_fd != -1;
        
        if(!listener_was_created)
        {
          if(callbackListenError)
            callbackListenError(_createErrorMessage(
              "Listen Error", "Failed to create listener"));
          listener_requested = false;
          using_pause = true;
          continue;
        }
        else if(!FD_ISSET(listen_socket_fd,&all_read_fds))
        {
          FD_SET(listen_socket_fd, &all_read_fds);
          using_pause = false;
          listener_available = true;
          peers_or_listener_available.notify_one();
        }
      }
      else if(changing_listener)//Listen port was changed 
      {
        if(FD_ISSET(listen_socket_fd,&all_read_fds))
        {
          FD_CLR(listen_socket_fd, &all_read_fds);
        }
          
        listener_available = false;
        changing_listener = false;
        close(listen_socket_fd);
        listen_socket_fd = -1;
        continue;
      }
      else if(!FD_ISSET(listen_socket_fd,&all_read_fds))
      {//Make sure listen_socket_fd is in set
        FD_SET(listen_socket_fd, &all_read_fds);
      }
    }
    else //Not accepting connections
    {//listen_socket_fd must be closed to really
     //ensure that no one can connect.
      if(listen_socket_fd != -1 ||
         listener_available)
      {
        if(FD_ISSET(listen_socket_fd, &all_read_fds))
        {
          FD_CLR(listen_socket_fd, &all_read_fds);
        }
        listener_available = false;
        close(listen_socket_fd);
        listen_socket_fd = -1;
      }
    }

    int number_of_fd_changes(0);
    greatest_fd = _addPeersToSetAndReturnBiggestFd(&all_read_fds);
    changed_read_fds = all_read_fds;

    if((greatest_fd <= 0) && using_pause) 
    {
      wakeup_listen_thread = false;
      _pauseUntilPeersOrListenerAvailable();
      continue;
    }

    number_of_fd_changes = _applySelectToReadFds(
            greatest_fd, &changed_read_fds);

    if(number_of_fd_changes < 0)//Error
    {
      if(callbackListenError)
      {
        callbackListenError(
          _createErrorMessage(
            "Listen Error",
            "Select returned -1")); 
      }
    }
    else if(number_of_fd_changes > 0)//Ready to receive or accept
    {
      _takeCareOfAllChangedReadFds(
        greatest_fd, &changed_read_fds, &all_read_fds);
    }
  }
}

void TcpNode::_takeCareOfAllChangedReadFds(
        int greatest_file_descriptor,
        fd_set *changed_file_descriptors,
        fd_set *master_fd_set)
{
  for(int i = 0; 
      i <= greatest_file_descriptor; 
      ++i)
  {
    if(FD_ISSET(i, changed_file_descriptors))
    {
      if(i == listen_socket_fd)//New connection
      {
        sockaddr_storage peer_addr;
        socklen_t addr_size = sizeof(peer_addr);
        int peer_socket_fd =
        accept(listen_socket_fd,
              (sockaddr*)&peer_addr,
              &addr_size);
        _addNewPeer(
            ++connection_counter,
            peer_socket_fd,
            _getRemoteIpAddress(peer_socket_fd),
            _getRemotePort(peer_addr));
        Lock lck(data_access);
        Peer newPeer(peers.back());
        lck.unlock();// Unlock for callback
        if(callbackNewPeerConnected) callbackNewPeerConnected(newPeer);
      }
      else//Data received
      {
        int32_t recbytes = receive_buffer_size;
        std::vector<char> buffer(recbytes);
        if(_receiveAllPossible(i,static_cast<char*>(buffer.data()), 
                              &recbytes,false))
        {
          Peer source = _getPeerBySocketFd(i);
          
          if(recbytes > 0)
          {
            if(callbackReceived) callbackReceived(source, buffer);
          }
          else //received bytes = 0 -> disconnect
          {
            if(callbackPeerDisconnected) callbackPeerDisconnected(source);
            FD_CLR(source.socket(), master_fd_set);
            disconnectPeer(source);
          }
        }
        else
        {
          if(callbackListenError)
          {
            callbackListenError(
              _createErrorMessage(
                "Listen Error", "recv() failed."));
          }
        }
      }
    }
  }
}

void TcpNode::_startListenThreadIfNotRunning()
{
  if(!listen_thread_running)
  {
    listen_thread_running = true;
    listenThread = 
    std::thread(&TcpNode::_listenThreadJob, this);
  }
}

void TcpNode::_connectThreadJob()
{
  size_t queue_length = 0;

  while(connect_thread_running)
  {
  {//Brackets limit life time of Lock()./////
    Lock lck(data_access);
    queue_length = potential_peers.size();
  }///////////////////////////////////////////
    
    if(queue_length <= 0)
    {
      _pauseUntilQueueNotEmpty();
      continue;
    }

    for(size_t i = 0; i < queue_length; ++i)
    {
      int peer_socket_fd = -1;
      bool success = false;
      
      addrinfo hints;
      addrinfo *result = NULL;
      memset(&hints, 0, sizeof(hints));
      Lock lck(data_access);
      
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      std::string hostip;
      uint16_t hostport;
      std::string hostport_str;
    
      hostip = potential_peers.front().first;
      hostport = potential_peers.front().second;
      hostport_str = std::to_string(hostport);
      lck.unlock();

      if(getaddrinfo(hostip.c_str(),
        hostport_str.c_str(),
        &hints, &result) == 0)
      {
        if(result)
        {
          peer_socket_fd = socket(result->ai_family, result->ai_socktype,
                                  result->ai_protocol);
          if(_changeBlocking(peer_socket_fd, true))
          {
            int conn = connect(peer_socket_fd,
                              result->ai_addr,
                              result->ai_addrlen);
            
            if(conn != 0)//See if in progress
            {
              if(errno == EINPROGRESS)
              {
                std::this_thread::sleep_for(
                  std::chrono::milliseconds(TIMEOUT_MS));
        
                int sock_error;
                socklen_t len = sizeof(sock_error);
                if(getsockopt(peer_socket_fd,
                              SOL_SOCKET,
                              SO_ERROR,
                              (void*)&sock_error,
                              &len) == 0)
                {
                  if(sock_error == 0)//Success after short wait
                  {
                    if(_changeBlocking(peer_socket_fd, false))
                    {
                      success = true;
                    }
                  }
                }
              }
            }
            else//Immediate success
            {
              success = true;
            }
          }
        }
      }

      if(success)
      {
        _addNewPeer(
            ++connection_counter, peer_socket_fd, hostip, hostport);
        lck.lock();
        potential_peers.pop_front();
        Peer newPeer = peers.back();
        lck.unlock();
        if(callbackConnectedToNewPeer) callbackConnectedToNewPeer(newPeer);
      }
      else
      {
        lck.lock();
        potential_peers.pop_front();
        lck.unlock();
        if(callbackConnectError) callbackConnectError(
          _createErrorMessage(
          "Connect Error",
          "Failed to connect to " +
          hostip + ":" + hostport_str));
      }
      freeaddrinfo(result);
    }
  }
  
}

int TcpNode::_applySelectToWriteFds(
        int peer_socket_fd,
        fd_set *write_set)
{
  timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = TIMEOUT_MS * 1000;
  FD_ZERO(write_set);
  FD_SET(peer_socket_fd, write_set);
  return select(
            peer_socket_fd + 1,
            NULL,
            write_set,
            NULL,
            &timeout);
}

void TcpNode::_sendThreadJob()
{
  size_t to_send_count(0);

  while(send_thread_running)
  {
   {//Brackets limit life time of Lock()./////
     Lock lck(data_access);
     to_send_count = data_to_send.size();
   }//////////////////////////////////////////
     if(to_send_count <= 0)
     {
       _pauseUntilDataToSendAvailable();
       continue;
     }
   
    for(size_t i = 0; i <to_send_count; ++i)
    {
      Lock lck(data_access);
      SocketOfOutBufferList::iterator curr_socket_and_buffer;
      curr_socket_and_buffer = data_to_send.begin();

      fd_set write_set;
      int used_socket_fd(-1);
        
      used_socket_fd = curr_socket_and_buffer->first;

      lck.unlock();//Prevent deadlock 
      Peer pr = _getPeerBySocketFd(used_socket_fd);
      lck.lock();

      if(!pr)
      {
        lck.unlock();// Unlock for callback ///////
        if(callbackSendError) callbackSendError(
          _createErrorMessage("Send Error",
          "Specified peer does not exist."));
        lck.lock();////////////////////////////////
        data_to_send.pop_front();
        break;
      }

      int fd_changes = _applySelectToWriteFds(
                used_socket_fd,
                &write_set);
      
      if(fd_changes >= 0)
      {
        if(FD_ISSET(used_socket_fd, &write_set))
        {
          std::vector<char> *curr_buffer(NULL);
          curr_buffer = &curr_socket_and_buffer->second;
          int32_t length = curr_buffer->size();
          
          if(_sendAllPossible(
              used_socket_fd, 
              curr_buffer->data(),
              &length, false))
          { //Erase data from buffer when it's sent
            curr_buffer->erase(curr_buffer->begin(), 
                              curr_buffer->begin() + length);
            lck.unlock();// Unlock for callback /////////////
            if(callbackSent) callbackSent(pr, length);
            lck.lock();///////////////////////////////
          }
          else
          {
            lck.unlock();// Unlock for callback ///////////
            if(callbackSendError) callbackSendError(
              _createErrorMessage("Send Error",
              "sendAllPossible() failed."));
            lck.lock();///////////////////////////////////
            data_to_send.pop_front();
            break;
          }
        }
        else
        {
          //Timeout
        }
      }
      else
      {
        lck.unlock(); // Unlock for callback /////////////
        if(callbackSendError) callbackSendError(
          _createErrorMessage("Send Error",
          "_applySelectToWriteFd failed."));
        lck.lock(); ///////////////////////////////////////
        data_to_send.pop_front();
        break;
      }

      ++curr_socket_and_buffer;
    }
    
    {//Brackets limit life time of Lock()./////
    Lock lck(data_access);
    data_to_send.remove_if(
      [](SocketOfOutBuffer &elem){return elem.second.empty();});
    }//////////////////////////////////////////
  }
}

void TcpNode::_pauseUntilPeersOrListenerAvailable()
{
  Lock lck(data_access);
  peers_or_listener_available.wait(
    lck, [this](){return !peers.empty() || 
    listener_available || destructor_called ||
    changing_listener || wakeup_listen_thread;});
}

void TcpNode::_pauseUntilQueueNotEmpty()
{
  Lock lck(data_access);
  queue_not_empty.wait(lck, [this](){
    return !potential_peers.empty() || destructor_called;});
}

void TcpNode::_pauseUntilDataToSendAvailable()
{
  Lock lck(data_access);
  data_to_send_available.wait(lck, [this](){
    return !data_to_send.empty() || destructor_called;});
}

Peer TcpNode::_getPeerBySocketFd(int sockfd)
{
  Lock lck(data_access);
  Peer tbr;
  for(Peer &pr : peers)
  {
    if(pr.socket() == sockfd)
    {
      tbr = pr;
      break;
    }
  }
  return tbr;
}


void TcpNode::disconnectPeer(Peer pr)
{
  Lock lck(data_access);
  if(!peers.empty())
  {
    int socket_fd_to_be_closed = -1;
    size_t i = 0;
    while(i < peers.size())
    {
      if(peers[i] == pr)
      {
        socket_fd_to_be_closed = peers[i].socket();
        peers.erase(peers.begin() + i);
        close(socket_fd_to_be_closed);
        break;
      }
      ++i;
    }
  }
}

void TcpNode::disconnectPeer(
      const std::string &ip,
      uint16_t port)
{
  Peer pr = _findPeerByIpAndPort(ip,port);
  if(pr)
  {
    close(pr.socket());
    Lock lck(data_access);
    peers.erase(std::remove(peers.begin(), 
                peers.end(), pr), peers.end());
  }
}

void TcpNode::disconnectPeer(uint64_t peerId)
{
  disconnectPeer(_findPeerById(peerId));
}

void TcpNode::disconnectAll()
{
  Lock lck(data_access);
  for(Peer &p : peers)
  {
    close(p.socket());
  }
  peers.clear();
}


void TcpNode::_addNewPeer(
  uint64_t conn_id, int socketfd, const std::string &ip, uint16_t p)
{
  Lock lck(data_access);
  peers.push_back(Peer(conn_id,socketfd, ip, p));
  lck.unlock();
  peers_or_listener_available.notify_one();
}

Message TcpNode::_createErrorMessage(
  const std::string &h,
  const std::string &b)
{
  Message m{h, b};
  if(errno != 0)
  {
    std::string error_number;
    error_number += "\nError number " + std::to_string(errno) + ": " +
                  std::string(strerror(errno));
    m.body += error_number;
  } 
  return m;
}

bool TcpNode::_peerInVector(const Peer &pr)
{
  Lock lck(data_access);
  bool tbr = false;
  for(Peer &p : peers)
  {
    if(p == pr)
    {
      tbr = true;
      break;
    }
  }
  return tbr;
}

Peer TcpNode::_findPeerByIpAndPort(
    const std::string &ip,
    uint16_t port)
{
  Lock lck(data_access);
  Peer tbr;
  for(Peer &p : peers)
  {
    if(p.ipAddress() == ip &&
       p.port() == port)
    {
      tbr = p;
      break;
    }
  }
  return tbr;
}

Peer TcpNode::_findPeerById(uint64_t peerId)
{
  Lock lck(data_access);
  Peer tbr;
  for(Peer &p : peers)
  {
    if(peerId == p.id())
    {
      tbr = p;
      break;
    }
  }  
  return tbr;
}

int TcpNode::
_createNewListener(IpVersion ipversion, uint16_t portnum)
{
  int listensock_fd(-1);
  addrinfo hints;
  addrinfo *result;

  //Make sure port cannot be 0
  if(portnum == 0)
  {
    return listensock_fd;
  }

  //Init hints with zeros
  memset(&hints, 0, sizeof(hints));

  ipversion != IpVersion::ANY ?
    ipversion == IpVersion::IPv4 ?
      hints.ai_family = AF_INET : 
      hints.ai_family = AF_INET6 
    :
    hints.ai_family = AF_UNSPEC;

  //SOCK_STREAM -> TCP
  hints.ai_socktype = SOCK_STREAM;
  //AI_PASSIVE -> local ip address
  hints.ai_flags = AI_PASSIVE;

  if(getaddrinfo(NULL,
      std::to_string(portnum).c_str(),
      &hints,
      &result) == 0)
  {
    //Store pointer to first element
    //(for later memory freeing)
    addrinfo *p_first = result;

    //Iterate through result linked list and try
    //to create socket, bind socket and listen on
    //socket until it succeeds.
    while(result != NULL)
    {
      listensock_fd =
          socket(result->ai_family,
                 result->ai_socktype,
                 result->ai_protocol);
      if(listensock_fd != -1)
      {
        int yes = 1;
        if(setsockopt(listensock_fd,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &yes, sizeof(yes)) == 0 &&
            bind(listensock_fd,
                 result->ai_addr,
                 result->ai_addrlen) == 0 &&
            listen(listensock_fd, 20) == 0)
        {
          break;
        }
        else
        {
          close(listensock_fd);
          listensock_fd = -1;
        }
      }

      result = result->ai_next;
    }

    freeaddrinfo(p_first);
  }

  return listensock_fd;
}


bool TcpNode::
_changeBlocking(int sockfd, bool nonblocking)
{
  bool to_be_returned = false;

  //Descriptor must be valid
  if(sockfd != -1)
  {
    long socket_settings;
    //First get the current settings
    socket_settings = fcntl(sockfd, F_GETFL, NULL);

    if(socket_settings != -1)
    {
      //Alter settings accordingly
      if(nonblocking)
      {
        socket_settings |= O_NONBLOCK;
      }
      else
      {
        socket_settings &= (~O_NONBLOCK);
      }

      //Apply altered settings
      if(fcntl(sockfd, F_SETFL, socket_settings) != -1)
      {
        to_be_returned = true;
      }
    }
  }

  return to_be_returned;
}


std::string TcpNode::_getOwnIpAddress()
{
  std::string addr;
  size_t len = 100;

  // Allocate some space for the
  // IP address string.
  // Guessing that 100 bytes should be more
  // than enough.
  char *c_string = new char[len];

  if(gethostname(c_string, len) != -1)
  {
    addr = std::string(c_string);
  }

  delete[] c_string;

  return addr;
}

std::string TcpNode::_getRemoteIpAddress(int sockfd)
{
  std::string addr;

  sockaddr address_info_storage;
  unsigned length = sizeof(sockaddr);
  if(getpeername(sockfd, &address_info_storage,
                 &length) == 0)
  {
    char *addr_string = new char[length +1];

    //Check IP version
    if(address_info_storage.sa_family == AF_INET)
    {//IPv4 is stored sockaddr_in
      sockaddr_in *ipv4 = (sockaddr_in*) &address_info_storage;
      inet_ntop(AF_INET, &(ipv4->sin_addr),
                addr_string, INET_ADDRSTRLEN);
    }
    else
    {// IPv6 is stored in sockaddr_in6
      sockaddr_in6 *ipv6 = (sockaddr_in6*) &address_info_storage;
      inet_ntop(AF_INET6, &(ipv6->sin6_addr),
                addr_string, INET6_ADDRSTRLEN);
    }

    addr = std::string(addr_string);
    delete[] addr_string;
  }

  return addr;
}


uint16_t TcpNode::_getRemotePort(
    const sockaddr_storage &addrstorage)
{
  uint16_t portnum = 0;

  if(addrstorage.ss_family == AF_INET)
  {
    sockaddr_in *ipv4 = (sockaddr_in*) &addrstorage;
    portnum = ntohs(ipv4->sin_port);
  }
  else if(addrstorage.ss_family == AF_INET6)
  {
    sockaddr_in6 *ipv6 = (sockaddr_in6*) &addrstorage;
    portnum = ntohs(ipv6->sin6_port);
  }

  return portnum;
}


bool TcpNode::_receiveAllPossible(
      int s, 
      char* buf,
      int32_t *len, 
      bool blocking)
{
  bool to_be_returned = false;
  int32_t total = 0;
  int32_t bytesleft = *len;
  int32_t n = 0;
  int option = 0;

  if(!blocking)
    option = MSG_DONTWAIT;

  bool data_available = true;

  while(total < *len && data_available)
  {//Using recv() with the MSG_PEEK
   //flag to see whether data is
   //available.
    data_available =
        recv(s, buf+total,
             bytesleft,
             MSG_PEEK | option) > 0;

    if(data_available)
    {
      n = recv(s, buf+total, bytesleft, option);
      if(n == -1) { break; }
      total += n;
      bytesleft -= n;
    }
  }

  *len = total;

  if((n == -1))
  { 
    if(!blocking)
    {//If recv() failed only because the operation would block
    //then this is not considered an error
      if(errno == EAGAIN || errno == EWOULDBLOCK)
        to_be_returned = true;
    }
  }
  else
  {
    to_be_returned = true;
  }

  return to_be_returned;
}


bool TcpNode::_sendAllPossible(
      int s, 
      const char* buf,
      int32_t *len, 
      bool blocking)
{
  bool to_be_returned = false;
  int32_t total = 0;        // how many bytes were sent
  int32_t bytesleft = *len; // how many are left to send
  int32_t n;
  int option = 0;

  if(!blocking)
    option = MSG_DONTWAIT;

  while(total < *len)
  {
    n = send(s, buf+total, bytesleft, option);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }

  *len = total; // return number actually sent here

  if((n == -1) && !blocking)
  { //If send() failed only because the operation would block
    //then this is not considered an error
    if(errno == EAGAIN || errno == EWOULDBLOCK)
      to_be_returned = true;
  }
  else
  {
    to_be_returned = true;
  }

  return to_be_returned; // return false on failure, true on success
}

}

