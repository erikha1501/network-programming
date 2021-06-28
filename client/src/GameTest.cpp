#include "GameTest.hpp"

#include <google/protobuf/util/json_util.h>

#include <poll.h>
#include <stdio.h>

GameTest::GameTest(size_t messageBufferSize)
    : m_bufferSize(messageBufferSize), m_buffer(new uint8_t[messageBufferSize]), m_socket(true)
{
    m_clientID = 0;

    m_directionX = 0.0f;
    m_directionY = 0.0f;
}

void GameTest::start()
{
    IPEndpoint server{INADDR_LOOPBACK, 6970};

    pollfd pollList[2] = {{.fd = STDIN_FILENO, .events = POLLIN}, {.fd = m_socket.socketFd(), .events = POLLIN}};

    int choice = -1;

    while (true)
    {
        poll(pollList, 2, -1);

        if ((pollList[0].revents & POLLIN) != 0)
        {
            scanf(" %d", &choice);

            switch (choice)
            {
            case 69: {
                uint8_t* buffer = m_buffer.get();
                buffer[0] = 69;
                m_socket.send(buffer, 1, server);
                break;
            }
            case 70: {
                scanf(" %f %f", &m_directionX, &m_directionY);
            }

            default: {
                MessageType messageType = (MessageType)choice;
                switch (messageType)
                {
                case MessageType::PlayerInput: {
                    pnet::PlayerInput playerInput{};
                    playerInput.mutable_player_identity()->set_client_id(m_clientID);
                    playerInput.mutable_player_identity()->set_room_id(0);
                    playerInput.set_direction_x(m_directionX);
                    playerInput.set_direction_y(m_directionY);

                    sendMessage(server, MessageType::PlayerInput, playerInput);
                    break;
                }

                default:
                    break;
                }
                break;
            }
            }
        }

        if ((pollList[1].revents & POLLIN) != 0)
        {
            ssize_t read = m_socket.recv(m_buffer.get(), m_bufferSize, server);

            if (read == -1)
            {
                printf("Socket read error: errorno %d", errno);
                break;
            }

            if (read >= m_bufferSize)
            {
                continue;
            }

            handleMessage(server, m_buffer.get(), read);
        }
    }
}

void GameTest::sendMessage(const IPEndpoint& receiver, MessageType type, const google::protobuf::Message& message)
{
    uint8_t* buffer = m_buffer.get();

    // Write message type
    buffer[0] = (uint8_t)type;

    // Write message
    size_t messageSize = message.ByteSizeLong();
    bool ret = message.SerializeToArray(buffer + 1, messageSize);
    assert(ret);

    m_socket.send(buffer, messageSize + 1, receiver);
}

static void printProtoAsJson(const google::protobuf::Message& proto)
{
    namespace pu = google::protobuf::util;

    std::string json;
    pu::JsonOptions jsonOptions;
    jsonOptions.always_print_primitive_fields = true;
    jsonOptions.add_whitespace = true;
    pu::MessageToJsonString(proto, &json, jsonOptions);

    printf("%s\n", json.c_str());
}

void GameTest::handleMessage(const IPEndpoint& sender, const uint8_t* buffer, size_t size)
{
    // Read message type
    MessageType mesgType = (MessageType)buffer[0];
    size_t messageSize = size - 1;
    buffer += 1;

    switch (mesgType)
    {
    case MessageType::GameState: {
        pnet::GameState gameState{};
        bool parseResult = gameState.ParseFromArray(buffer, messageSize);
        assert(parseResult);

        const pnet::PlayerState& playerState = gameState.player_states()[0];
        printf("\r(%4.2f, %4.2f) (%4.2f, %4.2f)       ", playerState.position_x(), playerState.position_y(),
               playerState.direction_x(), playerState.direction_y());
        break;
    }

    default:
        break;
    }
}