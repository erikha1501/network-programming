#pragma once

#include "../../server/src/IPEndpoint.hpp"
#include "../../server/src/MessageType.hpp"
#include "../../server/src/UDPSocket.hpp"

#include "../../protobuf/src/pnet.pb.h"

#include <memory>
#include <string>

class GameTest
{
public:
    GameTest(size_t messageBufferSize = 1024);

    void start();

private:
    void handleMessage(const IPEndpoint& sender, const uint8_t* buffer, size_t size);

    void sendMessage(const IPEndpoint& receiver, MessageType type, const google::protobuf::Message& message);

private:
    int m_bufferSize;
    std::unique_ptr<uint8_t[]> m_buffer;

    int m_clientID;

    float m_directionX;
    float m_directionY;

    UDPSocket m_socket;
};