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


#include <iostream>
#include <cstring>
#include <algorithm> 
#include "../include/simpwire.hpp"

//Command strings
constexpr char VERSION[] = "Version";
constexpr char CONNECT[] = "Connect";
constexpr char SEND[] = "Send";
constexpr char LISTEN[] = "Listen";
constexpr char IS_LISTENING[] = "Is_listening"; 
constexpr char SHOW_LISTEN_PORT[] = "Show_listen_port"; 
constexpr char SET_RECEIVE_BUFFER_SIZE[] = "Set_receive_buffer_size";
constexpr char SHOW_RECEIVE_BUFFER_SIZE[] = "Show_receive_buffer_size";
constexpr char SHOW_LATEST_PEER[] = "Show_latest_peer"; 
constexpr char STOP_LISTENING[] = "Stop_listening";
constexpr char DISCONNECT[] = "Disconnect";
constexpr char DISCONNECT_ALL[] = "Disconnect_all";
constexpr char SHOW_PEERS[] = "Show_peers";
constexpr char EXIT[] = "Exit";
constexpr char LIST_COMMANDS[] = "List_commands";

//IP version identifiers
constexpr char IPv4[] = "IPv4";
constexpr char IPv6[] = "IPv6";

typedef std::vector<std::string> StringVec;

// ########## Helper functions ###################
void tokenize(const std::string &input, 
              StringVec &output, 
              const std::string &separators)
{
    output.clear();
    //alloc memory for c string (strtok does not accept input.c_str())
    char *c_string = new char[input.length()+1];
    //copy input string into c string
    std::strcpy(c_string, input.c_str());
    char *tok = nullptr;
    //get first substring
    tok = std::strtok(c_string, separators.c_str());

    //get remaining substrings
    while(tok != nullptr)
    {
        //store substring
        output.push_back(std::string(tok));
        //strtok expects NULL parameter after first substring was returned
        tok = strtok(nullptr, separators.c_str());
    }

    //free memory
    delete[] c_string;
}


void listCommands()
{
    std::cout << "> Available commands: " << std::endl
    << "> - " << VERSION << std::endl
    << "> - " << CONNECT << " <ip> <port> " << std::endl
    << "> - " << SEND << " <id> <data> " << std::endl
    << "> - " << LISTEN << " <port> " 
    << " [" << IPv4 << "|" << IPv6 << "] "    << std::endl
    << "> - " << IS_LISTENING << std::endl
    << "> - " << SHOW_LISTEN_PORT << std::endl
    << "> - " << SET_RECEIVE_BUFFER_SIZE << " <size> " << std::endl
    << "> - " << SHOW_RECEIVE_BUFFER_SIZE << std::endl
    << "> - " << SHOW_LATEST_PEER << std::endl
    << "> - " << STOP_LISTENING << std::endl
    << "> - " << DISCONNECT << " <id> " << std::endl
    << "> - " << DISCONNECT_ALL << std::endl
    << "> - " << SHOW_PEERS << std::endl
    << "> - " << LIST_COMMANDS << std::endl
    << "> - " << EXIT << std::endl << std::endl;
}


std::string bytesToString(const std::vector<uint8_t> &bv)
{
    return std::string(bv.begin(), bv.end());
}

std::vector<uint8_t> stringToBytes(const std::string &s)
{
    return std::vector<uint8_t>(s.begin(), s.end());
}

void reportError(spw::Message msg)
{
    std::cerr << "> "<< msg.head << std::endl
              << "> " << msg.body << std::endl;
}

spw::Peer peerAt(uint64_t id, std::unordered_map<uint64_t,spw::Peer> peers)
{
    if(peers.find(id) != peers.end())
    {
        return peers[id];
    }
    else
    {
        return spw::Peer();
    }
}

// ######## main #######################################
int main(int argc, char **argv)
{

{
    bool exit_program = false;
    spw::TcpNode node;
    
    std::string command;
    StringVec cmd_tokens;

    //Setup callbacks
    node.onListenError(reportError);
    node.onSendError(reportError);
    node.onConnectError(reportError);
    node.onFaultyConnectionClosed([](spw::Peer pr, spw::Message errmsg){
        std::cout << "Closed connection to " << pr.ipAddress() << ":"
        << std::to_string(pr.port()) << " due to error: " << 
        errmsg.head << ": " << errmsg.body << std::endl;
    });

    node.onAccept([&](spw::Peer pr){
        std::cout << "> " << "(" << pr.id() << ") "
                            << pr.ipAddress() << ":" << pr.port() 
                            << " has connected." << std::endl;
    });

    node.onReceive([&](spw::Peer pr, std::vector<uint8_t> data){

        std::vector<uint8_t> copy_data = data;

        //Remove last occurence of newline
        auto newlinePos = std::find(copy_data.rbegin(),copy_data.rend(),'\n');
        if(newlinePos != copy_data.rend())
        {
            copy_data.erase(--(newlinePos.base()));
        }


        std::cout << "> Received " << strlen(reinterpret_cast<char*>(copy_data.data())) << " bytes from: "
                  << "(" << pr.id() << ") "
                  << pr.ipAddress() << ":" << pr.port() << ": "
                  << bytesToString(copy_data) << std::endl;
    });

    node.onDisconnect([](spw::Peer pr){
        std::cout << "> " << "(" << pr.id() << ") "
                  << pr.ipAddress() << ":" << pr.port() 
                  << " has disconnected." << std::endl; 
    });

    node.onClosedConnection([](spw::Peer pr) {
        std::cout << "> Closed connection to " << "(" << pr.id() << ") "
                  << pr.ipAddress() << ":" << pr.port() << std::endl;
    });


    node.onConnect([](spw::Peer pr){
        std::cout << "> Connected to " << "(" << pr.id() << ") "    
                  << pr.ipAddress() << ":" << pr.port()
                  << std::endl;
    });
    
    node.onStartedListening([](uint16_t port){
        std::cout << "> Started listening on port " << port << "." << std::endl;
    });

    node.onStoppedListening([](){
      std::cout << "> Stopped listening." << std::endl;
    });

    StringVec tokens;
    std::string progname;
    tokenize(std::string(argv[0]), tokens, "/" );
    progname = tokens.back();

    std::cout << "> ### Welcome to " << progname << " ###" << std::endl;
    listCommands();

    //Main loop
    while(!exit_program)
    {
        getline(std::cin, command);
        tokenize(command,cmd_tokens," ");
        
        if(cmd_tokens.size() <= 0)
        {
            std::cerr << "> Error: Command empty." << std::endl;
            continue;
        }

        if(cmd_tokens[0] == std::string(VERSION))
        {
            std::cout << "Version: " << spw::version() << std::endl;
        }
        else if(cmd_tokens[0] == std::string(CONNECT))
        {
            if(cmd_tokens.size() >= 3)
            {
                try
                {
                    node.connectTo(
                        cmd_tokens[1],
                        (uint16_t)std::stoi(cmd_tokens[2]));
                }
                catch(std::invalid_argument &e)
                {
                    std::cerr << "> Specified id could not be parsed." << std::endl;
                }
                catch(std::out_of_range &e)
                {
                    std::cerr << "> Specified id is to large." << std::endl;
                }

            }
            else
            {
                std::cerr << "> Error: " << CONNECT 
                          << " needs ip and port." << std::endl;
            }
        }
        else if(cmd_tokens[0] == std::string(SEND))
        {
            if(cmd_tokens.size() >= 3)
            {
                std::string dataToSend;
                for(size_t i = 2; i < cmd_tokens.size(); ++i)
                {
                    dataToSend += cmd_tokens[i];
                    dataToSend += " ";
                }

                //Remove last space from string
                dataToSend = dataToSend.substr(0, dataToSend.size()-1);
                
                try
                {
                    std::vector<uint8_t> conv_data(dataToSend.begin(), dataToSend.end());
                    uint64_t peer_id = static_cast<uint64_t>(std::stoi(cmd_tokens[1]));
                    node.sendData(peerAt(peer_id, node.allPeers()), conv_data);
                }
                catch(std::invalid_argument &e)
                {
                    std::cerr << "> Specified id could not be parsed." << std::endl;
                }
                catch(std::out_of_range &e)
                {
                    std::cerr << "> Specified id is to large." << std::endl;
                }
            }
            else
            {
                std::cerr << "Error: " << SEND 
                          << " needs id and data." << std::endl;
            }
        }
        else if(cmd_tokens[0] == std::string(LISTEN))
        {
            if(cmd_tokens.size() >= 2)
            {
                uint16_t requested_port(0);
                try
                {
                    requested_port = (uint16_t)std::stoi(cmd_tokens[1]);

                    if(cmd_tokens.size() >= 3)
                    {
                        if(cmd_tokens[2] == IPv4)
                        {
                            node.doListen(requested_port, spw::IpVersion::IPV4);
                        }
                        else if(cmd_tokens[2] == IPv6)
                        {
                            node.doListen(requested_port, spw::IpVersion::IPV6);
                        }
                        else
                        {
                            std::cerr << "> Specified IP version could not be parsed." 
                                                << std::endl;
                        }
                    }
                    else
                    {
                        node.doListen(requested_port);
                    }
                }
                catch(std::invalid_argument &e)
                {
                    std::cerr << "> Specified id could not be parsed." << std::endl;
                }
                catch(std::out_of_range &e)
                {
                    std::cerr << "> Specified id is to large." << std::endl;
                }

            }
            else
            {
                std::cerr << "> Port must be specified." << std::endl;
            }
        }
        else if(cmd_tokens[0] == std::string(IS_LISTENING))
        {
            if(node.isListening())
                std::cout << "> Listening for connections." << std::endl;
            else
                std::cout << "> Not listening." << std::endl;
        }
        else if(cmd_tokens[0] == std::string(SHOW_LISTEN_PORT))
        {
            std::cout << "> " << node.listenPort() << std::endl;
        }
        else if(cmd_tokens[0] == std::string(SET_RECEIVE_BUFFER_SIZE))
        {
            if(cmd_tokens.size() >= 2)
            {
                try
                {
                    node.setReceiveBufferSize((size_t)std::stoi(cmd_tokens[1]));
                }
                catch(std::invalid_argument &e)
                {
                    std::cerr << "> Specified id could not be parsed." << std::endl;
                }
                catch(std::out_of_range &e)
                {
                    std::cerr << "> Specified id is to large." << std::endl;
                }
            }
            else
            {
                std::cerr << "> Size needs to be specified." << std::endl;
            }
        }
        else if (cmd_tokens[0] == std::string(SHOW_RECEIVE_BUFFER_SIZE))
        {
            std::cout << "> Receive buffer size: " << node.receiveBufferSize() 
                      << " bytes." << std::endl; 
        }
        else if (cmd_tokens[0] == std::string(SHOW_LATEST_PEER))
        {
            spw::Peer latestPeer = node.latestPeer();
            std::cout << "> " << " (" << latestPeer.id() << ") " 
                      << latestPeer.ipAddress() << ":" << latestPeer.port() 
                      << std::endl;
        }
        else if(cmd_tokens[0] == std::string(STOP_LISTENING))
        {
            node.stopListening();
        }
        else if(cmd_tokens[0] == std::string(DISCONNECT))
        {
            if(cmd_tokens.size() >= 2)
            {
                try
                {
                    uint64_t peer_id = static_cast<uint64_t>(std::stoi(cmd_tokens[1]));
                    node.disconnectPeer(peerAt(peer_id, node.allPeers()));
                }
                catch(std::invalid_argument &e)
                {
                    std::cerr << "> Specified id could not be parsed." << std::endl;
                }
                catch(std::out_of_range &e)
                {
                    std::cerr << "> Specified id is to large." << std::endl;
                }

            }
        }
        else if(cmd_tokens[0] == std::string(DISCONNECT_ALL))
        {
            node.disconnectAll();
        }
        else if(cmd_tokens[0] == std::string(EXIT))
        {
            exit_program = true;
        }
        else if(cmd_tokens[0] == std::string(SHOW_PEERS))
        {
            auto all_peers = node.allPeers();
            if(!all_peers.empty())
            {
                for(auto &p : all_peers)
                {
                    std::cout << "> " << "(" << p.second.id() << ")" << " " 
                              << p.second.ipAddress() << ":" 
                              << p.second.port() << std::endl;
                }
            }
            else
            {
                std::cout << "> No peers available." << std::endl;
            }

        }
        else if(cmd_tokens[0] == std::string(LIST_COMMANDS))
        {
            listCommands();
        }
        else
        {
            std::cerr << "> Error invalid command" << std::endl;
        }
    }
    
    std::cout << "> Destroying TcpNode..." << std::endl;
}

    std::cout << "> Program Finished" << std::endl;

    return 0;
}

