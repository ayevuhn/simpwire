/*
Copyright (c) 2019 Ivan Brebric

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

#include "gtest/gtest.h"
#include "../src/Socket.hpp"
#include <iostream>

using namespace testing;

constexpr int TEST_PORT = 23100;
static std::vector<uint8_t> test_data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

TEST(socket, canListenAndConnectIPv4)
{
  spw::Socket server;
  spw::Socket client;
  spw::Socket *peer = nullptr;

  bool listenOk = server.listen(TEST_PORT,spw::IpVersion::IPV4);

  ASSERT_TRUE(listenOk);
  ASSERT_TRUE(server.isListener());
  ASSERT_TRUE(server.isListening());
  ASSERT_EQ(server.listenPort(), TEST_PORT);

  client.connect("127.0.0.1", 23100);
  peer = dynamic_cast<spw::Socket*>(server.accept());

  ASSERT_TRUE(peer != nullptr);
  ASSERT_TRUE(client.isConnected());
  ASSERT_FALSE(client.isListener());
  ASSERT_FALSE(client.isListening());
  ASSERT_TRUE(peer->isConnected());
  ASSERT_FALSE(peer->isListener());
  ASSERT_FALSE(peer->isListening());

  client.close();
  peer->close();
  server.close();

  ASSERT_FALSE(client.isConnected());
  ASSERT_FALSE(peer->isConnected());
  ASSERT_FALSE(server.isListening());
  ASSERT_FALSE(server.isListener());

}

TEST(socket, canListenAndConnectIPv6)
{
  spw::Socket server;
  spw::Socket client;
  spw::Socket *peer = nullptr;

  bool listenOk = server.listen(TEST_PORT,spw::IpVersion::IPV6);

  ASSERT_TRUE(listenOk);
  ASSERT_TRUE(server.isListener());
  ASSERT_TRUE(server.isListening());
  ASSERT_EQ(server.listenPort(), TEST_PORT);

  bool connectOk = client.connect("localhost", 23100);
  peer = dynamic_cast<spw::Socket*>(server.accept());

  ASSERT_TRUE(peer != nullptr);
  ASSERT_TRUE(connectOk);
  ASSERT_TRUE(client.isConnected());
  ASSERT_TRUE(peer->isConnected());
  ASSERT_FALSE(peer->isListener());
  ASSERT_FALSE(peer->isListening());

  client.close();
  peer->close();
  server.close();

  ASSERT_FALSE(client.isConnected());
  ASSERT_FALSE(peer->isConnected());
  ASSERT_FALSE(server.isListening());
  ASSERT_FALSE(server.isListener());
}

TEST(socket, canSendAndReceiveIpV6)
{
  spw::Socket server;
  spw::Socket client;
  spw::Socket *peer = nullptr;

  std::vector<uint8_t> recv_data;

  server.listen(TEST_PORT,spw::IpVersion::IPV6);
  client.connect("localhost", TEST_PORT);
  peer = dynamic_cast<spw::Socket*>(server.accept());

  if(peer)
  {
    ASSERT_TRUE(peer->send(test_data));
  }

  ASSERT_EQ(client.receive(recv_data), spw::Socket::ReceiveResult::OK);
  ASSERT_EQ(test_data, recv_data);

  spw::Socket::ReceiveResult res = client.receive(recv_data);
  
  ASSERT_EQ(res, spw::Socket::ReceiveResult::ERROR_NOTHING_RECEIVED);

  peer->close();

  ASSERT_EQ(client.receive(recv_data),
            spw::Socket::ReceiveResult::ERROR_PEER_DISCONNECTED);

}

TEST(socket, canSendAndReceiveIpV4)
{
  spw::Socket server;
  spw::Socket client;
  spw::Socket *peer = nullptr;

  std::vector<uint8_t> recv_data;

  server.listen(TEST_PORT,spw::IpVersion::IPV4);
  client.connect("127.0.0.1", TEST_PORT);
  peer = dynamic_cast<spw::Socket*>(server.accept());

  if(peer)
  {
    ASSERT_TRUE(peer->send(test_data));
  }

  ASSERT_EQ(client.receive(recv_data), spw::Socket::ReceiveResult::OK);
  ASSERT_EQ(test_data, recv_data);

  ASSERT_EQ(client.receive(recv_data), 
            spw::Socket::ReceiveResult::ERROR_NOTHING_RECEIVED);

  peer->close();

  ASSERT_EQ(client.receive(recv_data),
            spw::Socket::ReceiveResult::ERROR_PEER_DISCONNECTED);
}

TEST(socket, canShowIpAndPortIpV6)
{
  spw::Socket server;
  spw::Socket client;
  spw::Socket *peer = nullptr;

  server.listen(TEST_PORT, spw::IpVersion::IPV6);
  client.connect("localhost", TEST_PORT);
  peer = dynamic_cast<spw::Socket*>(server.accept());

  std::string peer_ip_addr = client.peerIpAddress();
  uint16_t peer_port = client.peerPort();

  ASSERT_TRUE(!peer_ip_addr.empty());
  ASSERT_NE(peer_port, 0);

  std::string client_ip_addr = peer->peerIpAddress();
  uint16_t client_port = peer->peerPort();

  ASSERT_TRUE(!client_ip_addr.empty());
  ASSERT_NE(client_port, 0);

  std::string peer_name = client.peerName();
  ASSERT_FALSE(peer_name.empty());

  std::cout << "#### PEER IPv6 ADDRESS: " << peer_ip_addr << std::endl;
  std::cout << "#### PEER PORT: " << peer_port << std::endl;
  std::cout << "#### CLIENT IPv6 ADDRESS: " << client_ip_addr << std::endl;
  std::cout << "#### CLIENT PORT: " << client_port << std::endl;
  std::cout << "#### PEER NAME: " << peer_name << std::endl;
}

TEST(socket, canShowIpAndPortIpV4)
{
  spw::Socket server;
  spw::Socket client;
  spw::Socket *peer = nullptr;

  server.listen(TEST_PORT, spw::IpVersion::IPV4);
  client.connect("127.0.0.1", TEST_PORT);
  peer = dynamic_cast<spw::Socket*>(server.accept());

  std::string peer_ip_addr = client.peerIpAddress();
  uint16_t peer_port = client.peerPort();

  ASSERT_TRUE(!peer_ip_addr.empty());
  ASSERT_NE(peer_port, 0);
  
  std::string client_ip_addr = peer->peerIpAddress();
  uint16_t client_port = peer->peerPort();

  ASSERT_TRUE(!client_ip_addr.empty());
  ASSERT_NE(client_port, 0);

  std::string peer_name = client.peerName();
  ASSERT_FALSE(peer_name.empty());


  std::cout << "#### IPv4 ADDRESS: " << peer_ip_addr << std::endl;
  std::cout << "#### PORT : " << peer_port << std::endl;
  std::cout << "#### CLIENT IPv6 ADDRESS: " << client_ip_addr << std::endl;
  std::cout << "#### CLIENT PORT: " << client_port << std::endl;
  std::cout << "#### PEER NAME: " << peer_name << std::endl;
}
