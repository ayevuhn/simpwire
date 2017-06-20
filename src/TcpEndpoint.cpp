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
 * @file TcpEndpoint.cpp
 *
 * Contains the implementation of the class
 * TcpEndpoint
 */

#include "TcpEndpoint.h"

namespace spw
{


TcpEndpoint::
TcpEndpoint(const std::string &host_or_ip,
            uint16_t port) :

    is_server(false),
    connected(false),
    use_ip_v6(false),
    timeout_used(false),
    timeout_ms(DEFAULT_TIMEOUT),
    hostname_or_ip(host_or_ip),
    portnumber(port),
    listen_socket_fd(-1),
    comm_socket_fd(-1)
{

}

TcpEndpoint::
TcpEndpoint(uint16_t port) :
  TcpEndpoint("", port)
{

}

TcpEndpoint::
TcpEndpoint() :
  TcpEndpoint(DEFAULT_PORT)
{

}

TcpEndpoint::
~TcpEndpoint()
{
  close(comm_socket_fd);
  close(listen_socket_fd);
}


void TcpEndpoint::waitForConnection()
{

  //Terminate previous connection if any
  disconnect();

  if(!is_server)
  {
    is_server = createNewAcceptor();
  }

  if(is_server)
  {
    fd_set read_set;
    timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    timeval *p_timeout = NULL;
    if(timeout_used)
    {
      p_timeout = &timeout;
    }

    //Init read_set as empty
    FD_ZERO(&read_set);

    //Add listen_socket_fd to read_set
    FD_SET(listen_socket_fd, &read_set);

    //Start monitoring listen_socket_fd as read
    //descriptor
    int fd_changes = select(listen_socket_fd + 1,
           &read_set, NULL, NULL,
           p_timeout);

    //Since listen_socket_fd is the only monitored
    //descriptor, select() should return 1
    //(amount of detected changes) in case
    //of success
    if(fd_changes == 1)
    {
      if(FD_ISSET(listen_socket_fd, &read_set))
      {
        socklen_t addr_size = sizeof(remote_addr);
        comm_socket_fd =
            accept(listen_socket_fd,
                   (sockaddr*)&remote_addr,
                   &addr_size);
        connected = true;
      }
      else
      {
        throw OtherError();
      }

    }
    else if(fd_changes == 0)
    {
      throw Timeout("Timed out while waiting "\
                    "for connection.");
    }
  }
  else
  {
    throw OtherError();
  }
}



void TcpEndpoint::connectToHost()
{
  enum {ERROR, TIMEOUT, OK} op_result = ERROR;

  //Terminate previous connection if any
  disconnect();

  addrinfo hints;
  addrinfo *result;
  memset(&hints, 0, sizeof(hints));

  use_ip_v6 ?
      hints.ai_family = AF_INET6 :
      hints.ai_family = AF_INET;

  //SOCK_STREAM -> TCP
  hints.ai_socktype = SOCK_STREAM;

  if(getaddrinfo(hostname_or_ip.c_str(),
      std::to_string(portnumber).c_str(),
      &hints, &result) == 0)
  {
    if(result != NULL)
    {
      comm_socket_fd =
          socket(result->ai_family, result->ai_socktype,
                 result->ai_protocol);

      //Timeout used means that function may NOT block forever
      if(timeout_used)
      {
        op_result = TIMEOUT;
        uint32_t sleep_time = TIMESLICE;

        //Switch to non blocking mode for now
        if(changeBlocking(comm_socket_fd, true))
        {
          std::chrono::time_point<std::chrono::system_clock> start;
          std::chrono::duration<double> elapsed_chrono_type;
          uint32_t elapsed = 0;

          while(elapsed < timeout_ms)
          {
            start = std::chrono::system_clock::now();

            //Prevent too long sleep times
            uint32_t remaining = timeout_ms - elapsed;
            if(remaining < TIMESLICE)
              sleep_time = remaining;

            int conn = connect(comm_socket_fd,
                                result->ai_addr,
                                result->ai_addrlen);

            //connect() did not succeed immediately
            if(conn != 0)
            {
              //connect() is in progress
              if(errno == EINPROGRESS)
              {
                //Sleep to limit CPU usage
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(sleep_time));

                //Check whether connect() did succeed
                int sock_error;
                socklen_t len = sizeof(sock_error);
                if(getsockopt(comm_socket_fd,
                              SOL_SOCKET,
                              SO_ERROR,
                              (void*)&sock_error,
                               &len) == 0)
                {
                  if(sock_error == 0)
                  {
                    //Return to blocking mode
                    if(changeBlocking(comm_socket_fd, false))
                    {
                      connected = true;
                      op_result = OK;
                      break;
                    }
                  }
                }
              }
              else//connect() not in progress -> failed
              {
                //Limit CPU usage
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(sleep_time));
              }
            }
            else//connect() succeeded immediately
            {
              //Return to blocking mode
              if(changeBlocking(comm_socket_fd, false))
              {
                connected = true;
                op_result = OK;
                break;
              }
            }

            elapsed_chrono_type =
            (std::chrono::system_clock::now() - start);

            elapsed += elapsed_chrono_type.count()*1000;
          }
        }
      }
      else//No timeout -> function may block forever
      {
        if(connect(comm_socket_fd,
                   result->ai_addr,
                   result->ai_addrlen) == 0)
        {
          connected = true;
          op_result = OK;
        }
      }
    }

    freeaddrinfo(result);
  }

  // Throw exception if connect did not succeed.
  if(op_result != OK)
  {
    if(op_result == TIMEOUT)
    {
      throw Timeout("Timed out while trying to connect.");
    }
    else
    {
      throw OtherError();
    }
  }

}

int32_t TcpEndpoint::sendString(const std::string &data)
{
  return sendBytes(std::vector<char>(data.begin(),data.end()));
}

int32_t TcpEndpoint::sendBytes(const std::vector<char> &data)
{
  return sendBytes(static_cast<const char*>(data.data()),
            static_cast<int32_t>(data.size()));
}



int32_t TcpEndpoint::sendBytes(const char *data,
                               int32_t length)
{
  int32_t sent = 0;

  enum {ERROR,
        TIMEOUT,
        OK,
        NO_CONNECTION} op_result = ERROR;

  //Using this functions makes only sense if
  //a connection is established
  if(connected)
  {
    int used_socket_fd = comm_socket_fd;

    //Function may not block forever
    if(timeout_used)
    {
      //Prepare for select()
      fd_set write_set;
      timeval timeout;
      timeout.tv_sec = timeout_ms / 1000;
      timeout.tv_usec = (timeout_ms % 1000) * 1000;

      FD_ZERO(&write_set);
      FD_SET(used_socket_fd, &write_set);

      //Poll with select() and use timeout
      int fd_changes = select(used_socket_fd + 1,
                              NULL, &write_set,
                              NULL, &timeout);

      if(fd_changes >= 0)
      {
        //Ready to send
        if(FD_ISSET(used_socket_fd, &write_set))
        {
          //length = send(used_socket_fd, data, length, 0);
          if(sendAllPossible(used_socket_fd, data, &length, false))
          {
            if(length >= 0)
            {
              op_result = OK;
              sent = length;
            }
          }
        }
        else//Not ready to send
        {
          op_result = TIMEOUT;
        }
      }
    }
    else//Function may block forever
    {
      if(sendAllPossible(used_socket_fd, data, &length, true))
      {
        op_result = OK;
      }
    }
  }
  else
  {
    op_result = NO_CONNECTION;
  }

  // Throw exception if sendBytes did not succeed
  if(op_result != OK)
  {
    switch(op_result)
    {
      case NO_CONNECTION:
      {
        throw NoConnection("Tried to send bytes without connection.");
        break;
      }
      case TIMEOUT:
      {
        throw Timeout("Timed out while sending bytes.");
        break;
      }
      default:
      {
        throw OtherError();
        break;
      }
    }
  }



  return sent;
}


int32_t TcpEndpoint::receiveString(std::string &buffer,
                                   int32_t length)
{
  buffer.clear();
  std::vector<char> storage;
  int32_t byte_count = 0;

  byte_count = receiveBytes(storage,length);
  buffer = std::string(storage.begin(), storage.end());
  return byte_count;
}

int32_t TcpEndpoint::receiveBytes(std::vector<char> &buffer,
                                  int32_t length)
{
  buffer.clear();
  buffer.resize(length);

  return receiveBytes(static_cast<char*>(buffer.data()),
                      length);
}

int32_t TcpEndpoint::receiveBytes(char *buffer,
                                  int32_t length)
{
  int32_t received = 0;

  enum {ERROR,
        TIMEOUT,
        OK,
        DISCONNECT,
        NO_CONNECTION} op_result = ERROR;

  int used_socket_fd = comm_socket_fd;

  //Using this functions makes only sense if
  //a connection is established
  if(connected)
  {
    //Function may not block forever
    if(timeout_used)
    {
      //Prepare for select()
      fd_set read_set;
      timeval timeout;
      timeout.tv_sec = timeout_ms / 1000;
      timeout.tv_usec = (timeout_ms % 1000) * 1000;

      FD_ZERO(&read_set);
      FD_SET(used_socket_fd, &read_set);

      //Poll with select() and use timeout
      int fd_changes = select(used_socket_fd + 1,
                              &read_set, NULL,
                              NULL, &timeout);

      if(fd_changes >= 0)
      {
        if(FD_ISSET(used_socket_fd, &read_set))
        {
          int32_t recbytes = length;

          if(receiveAllPossible(used_socket_fd,
                                buffer, &recbytes, false))
          {
            if(recbytes > 0)
            {
              op_result = OK;
              received = recbytes;
            }
            else if(recbytes == 0)
            {
              op_result = DISCONNECT;
              disconnect();
            }
          }
        }
        else
        {
          op_result = TIMEOUT;
        }
      }
    }
    else//Function may block forever
    {
      int recbytes = length;

      if(receiveAllPossible(used_socket_fd,
                            buffer, &recbytes, true))
      {
        if(recbytes > 0)
        {
          op_result = OK;
        }
        else if(recbytes == 0)
        {
          op_result = DISCONNECT;
          disconnect();
        }
      }
    }
  }
  else
  {
    op_result = NO_CONNECTION;
  }

  //Throw exception if receiveBytes() failed
  if(op_result != OK)
  {
    switch(op_result)
    {
      case NO_CONNECTION:
      {
        throw NoConnection("Tried to send bytes without connection.");
        break;
      }
      case TIMEOUT:
      {
        throw Timeout("Timed out while receiving bytes.");
        break;
      }
      case DISCONNECT:
      {
        throw Disconnect("Peer disconnected while receiving.");
        break;
      }
      default:
      {
        throw OtherError();
        break;
      }
    }
  }

  return received;
}


std::string
TcpEndpoint::getPeerIpAddress()
{
  std::string addr;

  if(!connected)
  {
    throw NoConnection("Tried to get peer IP adress without connection.");
  }

  sockaddr sockad; //storage for address information
  unsigned length = sizeof(sockaddr);
  if(getpeername(comm_socket_fd,
                 &sockad, &length) == 0)
  {
    char *addr_string = new char[length +1];

    //Check IP version
    if(sockad.sa_family == AF_INET)
    {//IPv4 is stored sockaddr_in
      sockaddr_in *ipv4 = (sockaddr_in*) &sockad;
      inet_ntop(AF_INET, &(ipv4->sin_addr),
                addr_string, INET_ADDRSTRLEN);
    }
    else
    {// IPv6 is stored in sockaddr_in6
      sockaddr_in6 *ipv6 = (sockaddr_in6*) &sockad;
      inet_ntop(AF_INET6, &(ipv6->sin6_addr),
                addr_string, INET6_ADDRSTRLEN);
    }

    addr = std::string(addr_string);
    delete[] addr_string;
  }
  else
  {
    throw OtherError();
  }

  return addr;
}


uint16_t TcpEndpoint::getPeerPort()
{
  uint16_t portnum = 0;

  if(connected)
  {
    if(is_server)
    {
      if(remote_addr.ss_family == AF_INET)
      {
        sockaddr_in *ipv4 = (sockaddr_in*) &remote_addr;
        portnum = ntohs(ipv4->sin_port);
      }
      else
      {
        sockaddr_in6 *ipv6 = (sockaddr_in6*) &remote_addr;
        portnum = ntohs(ipv6->sin6_port);;
      }
    }
    else
    {//If not server then portnumber
     //is port of peer.
      portnum = portnumber;
    }
  }
  else
  {
    throw NoConnection("Tried to get peer port without connection.");
  }

  return portnum;
}

std::string TcpEndpoint::getOwnIpAddress()
{
  std::string addr;
  size_t len = 100;

  // Allocate some space for the
  // IP address string.
  // Guessing that 100 bytes should be more
  // than enough.
  char *c_string = new char[len];

  if(gethostname(c_string, len) == -1)
  {
    throw OtherError();
  }

  addr = std::string(c_string);
  delete[] c_string;

  return addr;
}


bool TcpEndpoint::
createNewAcceptor()
{
  int success = false;
  addrinfo hints;
  addrinfo *result;

  //Init hints with zeros
  memset(&hints, 0, sizeof(hints));

  use_ip_v6 ?
      hints.ai_family = AF_INET6 :
      hints.ai_family = AF_INET;

  //SOCK_STREAM -> TCP
  hints.ai_socktype = SOCK_STREAM;
  //AI_PASSIVE -> local ip address
  hints.ai_flags = AI_PASSIVE;

  if(getaddrinfo(NULL,
      std::to_string(portnumber).c_str(),
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
      listen_socket_fd =
          socket(result->ai_family,
                 result->ai_socktype,
                 result->ai_protocol);
      if(listen_socket_fd != -1)
      {
        int yes = 1;
        if(setsockopt(listen_socket_fd,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &yes, sizeof(yes)) == 0 &&
            bind(listen_socket_fd,
                 result->ai_addr,
                 result->ai_addrlen) == 0 &&
            listen(listen_socket_fd, DEFAULT_BACKLOG) == 0)
        {

          success = true;
          break;
        }
        else
        {
          close(listen_socket_fd);
          listen_socket_fd = -1;
        }
      }

      result = result->ai_next;
    }

    freeaddrinfo(p_first);
  }

  return success;
}


bool TcpEndpoint::
changeBlocking(int sockfd, bool nonblocking)
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
      if(fcntl(comm_socket_fd, F_SETFL, socket_settings) != -1)
      {
        to_be_returned = true;
      }
    }
  }

  return to_be_returned;
}


void TcpEndpoint::disconnect()
{
  close(comm_socket_fd);
  comm_socket_fd = -1;
  connected = false;
}

bool TcpEndpoint::sendAllPossible(int s, const char* buf,
                                  int32_t *len, bool blocking)
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

bool TcpEndpoint::receiveAllPossible(int s, char* buf,
                                     int32_t *len, bool blocking)
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

  if((n == -1) && !blocking)
  { //If recv() failed only because the operation would block
    //then this is not considered an error
    if(errno == EAGAIN || errno == EWOULDBLOCK)
      to_be_returned = true;
  }
  else
  {
    to_be_returned = true;
  }

  return to_be_returned;

}

}
