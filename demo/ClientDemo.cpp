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
* @file ClientDemo.cpp
*
* Short demonstration that shows how
* to use the TcpEndpoint class as a
* client.
*/

#include <iostream>
#include "TcpEndpoint.h"

/**
* At first this program will try to connect to
* localhost on port 52200. As soon as a connection
* is established it will send 10 messages to the
* server containing "Message 0", "Message 1", 
* "Message 2" and so on. After each sent message
* it will try to receive a reply of up to 20 bytes
* length and display it. 
* This program will terminate after all 10
* messages were sent.
*/
int main ()
{
  //Connect to localhost on port 52200
  spw::TcpEndpoint client("127.0.0.1",52200); 
  client.setTimeout(3000); //3 seconds timeout
  client.useTimeout(true); //Use non blocking mode
  
  std::cout << "Trying to connect." << std::endl;
  
  
  // In this part the client is trying to connect to a server.
  
  for(;;)
  {
    try
    {
      client.connectToHost();
      std::cout << "Connected to " << client.getPeerIpAddress() 
                << ":" << client.getPeerPort() << std::endl;
      break;
    }
    catch(spw::Timeout &ex)
    {
      std::cout << "Not connected yet." << std::endl;
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
  
  for(int i = 0; i < 10; ++i)
  {
    try
    {
      std::cout << "Sending: Message " << i << std::endl;
      client.sendString("Message " + std::to_string(i));
      std::cout << "Receiving..." << std::endl;
      client.receiveString(received, 20);
      std::cout << "Received: " << received << std::endl;
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
      exit(3);
    }
    catch(...)
    {
      std::cerr << "Error occured. Cause unknown. Terminating." << std::endl;
      exit(4);
    }
  }
  
  std::cout << "Done" << std::endl;
  
  return 0;
}

