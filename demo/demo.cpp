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
* @file nodeDemo.cpp
*
* Short demonstration that shows how
* to use the TcpEndpoint class.
*/

#include <iostream>
#include "../src/TcpEndpoint.h"


int main ()
{
  spw::TcpEndpoint node;
  node.setTimeout(3000); //3 seconds timeout
  node.useTimeout(true); //Use non blocking mode
  
  std::cout << "Trying to connect to 127.0.0.1:52200." << std::endl;
  
  
  // In this part the node is trying to connect to a local process that
  // is listening on port 52200. The easiest way to create a process
  // like this is to use netcat: 
  // netcat -l 52200
  
  for(;;)
  {
    try
    {
      node.connectToHost("127.0.0.1", 52200);
      std::cout << "Connected to " << node.getLatestPeer()->ipAddress()
                << ":" << node.getLatestPeer()->port() << std::endl;
      break;
    }
    catch(spw::Exception &ex)
    {
      std::cout << ex.what() << std::endl;
      if(!ex.isTimeout())
      {
        std::cout << "Terminating..." << std::endl;
        exit(9);
      }
    }
  }
  
  // Connection is established. Communication takes places in this part.
  
  std::string received;
  
  for(int i = 0; i < 10; ++i)
  {
    try
    {
      //Send
      std::cout << "Sending: Message " << i << std::endl;
      node.sendString("Message " + std::to_string(i) +"\n");
      //Receive
      std::cout << "Receiving..." << std::endl;
      node.receiveString(received, 20);
      std::cout << "Received: " << received << std::endl;
    }
    catch(spw::Exception &ex)
    {
      std::cout << ex.what() << std::endl;
      if(ex.isDisconnect())
      {
       std::cout << "Moving on..." << std::endl;
       break;
      }
      else if(!ex.isTimeout())
      {
        std::cout << "Terminating..." << std::endl;
        exit(9);
      }
    }
  }
  
  if(node.peersAvailable())
  {
    std::cout << "Disconnecting " << node.getLatestPeer()->ipAddress()
    << ":" << node.getLatestPeer()->port() << std::endl;
    node.disconnectPeer();
  }

  // In this part the node will wait for an incoming connection.
  // Again netcat can be used: 
  // netcat 127.0.0.1 45123
  std::cout << "Listening on port 45123..." << std::endl;

  node.setListenPort(45123);
  for(;;)
  {
    try
    {
      node.waitForConnection();
      std::cout << "Connected to " << node.getLatestPeer()->ipAddress()
                << ":" << node.getLatestPeer()->port() << std::endl;
      break;
    }
    catch(spw::Exception &ex)
    {
      std::cout << ex.what() << std::endl;
      if(!ex.isTimeout())
      {
        std::cout << "Terminating." << std::endl;
        exit(4);
      }
    }
  }

  // Connection is established. Communication takes places in this part

  for(int i = 0; i < 10; ++i)
  {
    try
    {
      //Send
      std::cout << "Sending: Message " << i << std::endl;
      node.sendString("Message " + std::to_string(i) +"\n");
      //Receive
      std::cout << "Receiving..." << std::endl;
      node.receiveString(received, 20);
      std::cout << "Received: " << received << std::endl;
    }
    catch(spw::Exception &ex)
    {
      std::cout << ex.what() << std::endl;
      if(ex.isDisconnect())
      {
        break;
      }
      else if(!ex.isTimeout())
      {
        std::cout << "Terminating..." << std::endl;
        exit(9);
      }
    }
  }

  std::cout << "Done" << std::endl;
  
  return 0;
}

