#pragma once

#include "../../server/src/IPEndpoint.hpp"
#include "../../server/src/MessageType.hpp"
#include "../../server/src/UDPSocket.hpp"

#include "../../protobuf/src/pnet.pb.h"

#include <memory>
#include <string>

class Client
{
public:
    Client(size_t messageBufferSize = 1024);

    void start();

private:
    void handleMessage(const IPEndpoint& sender, const uint8_t* buffer, size_t size);

    void sendMessage(const IPEndpoint& receiver, MessageType type, const google::protobuf::Message& message);

    void askForName();

private:
    int m_bufferSize;
    std::unique_ptr<uint8_t[]> m_buffer;

    std::string m_name;
    ssize_t m_roomID;
    int m_clientID;

    UDPSocket m_socket;
};