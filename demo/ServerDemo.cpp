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
* @file ServerDemo.cpp
*
* Short demonstration that shows how
* to use the TcpEndpoint class as a
* server.
*/

#include <iostream>
#include "TcpEndpoint.h"


/**
* At first this program will wait for a connection on port
* 52200. As soon as a client has connected it will try to receive
* messages which have 10 bytes or less. When a message arrives
* It will be displayed and a response message containing the
* word "Acknowledged" will be sent to the client. This program
* ends when the client closes the connection.
*/
int main ()
{
  spw::TcpEndpoint server(52200); //Listen on port 52200
  server.setTimeout(3000); //3 seconds timeout
  server.useTimeout(true); //Use non blocking mode
  
  std::cout << "Waiting for connection." << std::endl;
  
  
  // In this part the server is waiting for a peer to connect.
  
  for(;;)
  {
    try
    {
      server.waitForConnection();
      std::cout << "Connected to " << server.getPeerIpAddress() 
                << ":" << server.getPeerPort() << std::endl;
      break;
    }
    catch(spw::Timeout &ex)
    {
      std::cout << "No connection yet." << std::endl;
    }
    catch(spw::OtherError &ex)
    {
      std::cerr << ex.what() << std::endl;
      exit(1);
    }
    catch(...)
    {
      std::cerr << "Error occurred. Cause unknown. Terminating." << std::endl;
      exit(2);
    }
  }
  
  // Connection is established. Communication takes places in this part.
  
  std::string received;
  
  for(;;)
  {
    try
    {
      std::cout << "Receiving..." << std::endl;
      server.receiveString(received, 10);
      std::cout << "Received: " << received << std::endl;
      std::cout << "Sending: Acknowledged" << std::endl;
      server.sendString("Acknowledged");
    }
    catch(spw::Timeout &ex)
    {
      std::cout << "Nothing received yet" << std::endl;
    }
    catch(spw::Disconnect &ex)
    {
      std::cerr << ex.what() << std::endl;
      break;
    }
    catch(spw::OtherError &ex)
    {
      std::cerr << ex.what() << std::endl;
      exit(4);
    }
    catch(...)
    {
      std::cerr << "Error occured. Cause unknown. Terminating." << std::endl;
    }
  }
  
  std::cout << "Done" << std::endl;
  
  return 0;
}

