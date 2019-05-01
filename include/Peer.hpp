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

/*
 * @file Peer.h
 *
 * Contains the declaration of the class
 * Peer.
 */

#ifndef SPW_PEER_HPP_
#define SPW_PEER_HPP_

#include <cstdint>
#include <string>

#include "common.hpp"

namespace spw
{

/**
 * @class Peer
 * @brief Holds all information about a connected peers.
 * 
 * Objects of the class TcpNode    
 * can have multiple connections at the same time. 
 * They use a vector of Peer objects to store 
 * the information of all the currently connected peers.
 * 
 * Each Peer object contains:
 * - A connection id
 * - The ip address of the peer.
 * - The port number the peer is using.
 * - The host name of the peer
 * - A valid flag, which indicates whether that
 *   particular Peer object was created properly.
 *   (Proper Peer objects can only be created by
 *    the TcpNode class)
 * 
 * A Peer object can convert to bool. So
 * it can be used in a conditional:
 * Peer pr;
 * if(pr)
 * { ... }
 *
 * Peer objects evaluate to false when
 * they're not valid.
*/



class TcpNode;
class PeerPrivate;

#ifdef _WIN32
class DLL_IMPORT_EXPORT Peer
#else
class Peer
#endif
{

friend class TcpNode;
friend class TcpNodePrivate;

public:

    Peer();
    Peer(const Peer &other);
    Peer(Peer&& other);
    virtual ~Peer();

    virtual uint64_t id() const;
    virtual std::string ipAddress() const;
    virtual uint16_t port() const;
    virtual std::string hostName() const;
    virtual bool isValid() const;

    Peer& operator=(const Peer &other);
    bool operator==(const Peer &other) const;
    explicit operator bool() const;

private:

    mutable PeerPrivate *m_private = nullptr;
};

using PeerList = std::unordered_map<uint64_t, Peer>;

}



#endif /* SPW_PEER_HPP_ */
