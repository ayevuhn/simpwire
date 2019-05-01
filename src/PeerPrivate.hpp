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

#ifndef SPW_PEER_PRIVATE_HPP_
#define SPW_PEER_PRIVATE_HPP_

#include <cstdint>
#include <string>

#include "../include/common.hpp"
#include "ISocket.hpp"
#include "../include/Peer.hpp"

namespace spw
{

enum DisconnectType {
    PEER_DISCONNECTED_THEMSELF,
    PEER_WAS_DISCONNECTED,
    PEER_WAS_DISCONNECTED_DUE_TO_ERROR
};

class PeerPrivate
{

friend class TcpNode;
friend class TcpNodePrivate;

public:

    PeerPrivate();
    virtual ~PeerPrivate();

    uint64_t id();
    std::string ipAddress();
    uint16_t port();
    std::string hostName();
    bool isValid();

    PeerPrivate& operator=(const PeerPrivate &other);
    bool operator==(const PeerPrivate &other);
    explicit operator bool();


protected:
    //Following functions are supposed to be called
    //by friend class TcpNode

    void set(
        uint64_t conn_id,
        const std::string &ip,
        uint16_t port,
        const std::string &hostname);
    void setValid(bool valid);
    void scheduleDelete(DisconnectType dt);
    bool toBeDeleted();
    void setSocket(ISocket *sock);
    ISocket* getSocket();
    DisconnectType disconnectType();
    void destroySocket();
    void setErrorMessage(Message err);
    Message getErrorMessage();

private:

    uint64_t m_connection_id;
    std::string m_ip_address;
    uint16_t m_port_number;
    std::string m_hostname;
    bool m_valid = false;
    bool m_to_be_deleted = false;
    ISocket *m_socket = nullptr;
    DisconnectType m_disconn = PEER_DISCONNECTED_THEMSELF;
    Message m_errmsg;

};

}

#endif //SPW_PEER_PRIVATE_HPP
