#include "Client.hpp"

#include <google/protobuf/util/json_util.h>

#include <poll.h>
#include <stdio.h>

Client::Client(size_t messageBufferSize)
    : m_bufferSize(messageBufferSize), m_buffer(new uint8_t[messageBufferSize]), m_socket(true)
{
    m_clientID = -1;
    m_roomID = -1;
}

void Client::askForName()
{
    char buf[128];

    printf("Enter name: ");
    fgets(buf, 128, stdin);

    m_name = buf;
}

void Client::start()
{
    IPEndpoint server{INADDR_LOOPBACK, 6969};

    pollfd pollList[2] = {{.fd = STDIN_FILENO, .events = POLLIN}, {.fd = m_socket.socketFd(), .events = POLLIN}};

    int choice = -1;

    askForName();

    while (true)
    {
        printf("\nWaiting for events...\n");

        poll(pollList, 2, -1);

        if ((pollList[0].revents & POLLIN) != 0)
        {
            scanf(" %d", &choice);
            MessageType messageType = (MessageType)choice;

            switch (messageType)
            {
            case MessageType::QueryRoomInfoRequest: {
                pnet::QueryRoomInfoRequest request{};
                request.set_not_full(false);

                printf("Sending QueryRoomInfoRequest\n");
                sendMessage(server, messageType, request);
                break;
            }
            case MessageType::CreateRoomRequest: {
                if (m_roomID != -1 || m_clientID != -1)
                {
                    printf("Already in a room\n");
                    break;
                }
  
                pnet::CreateRoomRequest request{};
                request.mutable_client_info()->set_name(m_name);
                request.mutable_client_info()->set_id(0);

                printf("Sending CreateRoomRequest\n");
                sendMessage(server, messageType, request);
                break;
            }
            case MessageType::JoinRoomRequest: {
                if (m_roomID != -1 || m_clientID != -1)
                {
                    printf("Already in a room\n");
                    break;
                }

                pnet::JoinRoomRequest request{};
                request.mutable_client_info()->set_name(m_name);
                request.mutable_client_info()->set_id(0);

                printf("Enter room id: ");
                int roomID;
                scanf(" %d", &roomID);

                request.set_room_id(roomID);

                printf("Sending JoinRoomRequest\n");
                sendMessage(server, messageType, request);
                break;
            }
            case MessageType::LeaveRoomRequest: {
                if (m_roomID == -1 || m_clientID == -1)
                {
                    printf("Not in a room!\n");
                    break;
                }

                pnet::LeaveRoomRequest request{};
                request.mutable_client_identity()->set_room_id(m_roomID);
                request.mutable_client_identity()->set_client_id(m_clientID);

                printf("Sending LeaveRoomRequest\n");
                sendMessage(server, messageType, request);
                break;
            }

            default:
                break;
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

            printf("Received %ld byte packet: %s:%d\n", read, server.addressAsStr(), server.port());
            handleMessage(server, m_buffer.get(), read);
        }
    }
}

void Client::sendMessage(const IPEndpoint& receiver, MessageType type, const google::protobuf::Message& message)
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

void Client::handleMessage(const IPEndpoint& sender, const uint8_t* buffer, size_t size)
{
    // Read message type
    MessageType mesgType = (MessageType)buffer[0];
    size_t messageSize = size - 1;
    buffer += 1;

    switch (mesgType)
    {
    case MessageType::QueryRoomInfoResponse: {
        pnet::QueryRoomInfoResponse response{};
        bool parseResult = response.ParseFromArray(buffer, messageSize);
        assert(parseResult);

        printf("QueryRoomInfoResponse:\n");

        printProtoAsJson(response);
        break;
    }
    case MessageType::CreateRoomResponse: {
        pnet::CreateRoomResponse response{};
        bool parseResult = response.ParseFromArray(buffer, messageSize);
        assert(parseResult);

        m_clientID = response.assigned_identity().client_id();
        m_roomID = response.assigned_identity().room_id();

        printf("CreateRoomResponse:\n");

        printProtoAsJson(response);
        break;
    }
    case MessageType::JoinRoomResponse: {
        pnet::JoinRoomResponse response{};
        bool parseResult = response.ParseFromArray(buffer, messageSize);
        assert(parseResult);

        m_clientID = response.assigned_identity().client_id();
        m_roomID = response.assigned_identity().room_id();

        printf("JoinRoomResponse:\n");

        printProtoAsJson(response);
        break;
    }
    case MessageType::LeaveRoomResponse: {
        pnet::LeaveRoomResponse response{};
        bool parseResult = response.ParseFromArray(buffer, messageSize);
        assert(parseResult);

        m_clientID = -1;
        m_roomID = -1;

        printf("LeaveRoomResponse:\n");

        printProtoAsJson(response);
        break;
    }
    case MessageType::RoomInfoChanged: {
        pnet::RoomInfoChanged response{};
        bool parseResult = response.ParseFromArray(buffer, messageSize);
        assert(parseResult);

        printf("RoomInfoChanged:\n");

        printProtoAsJson(response);
        break;
    }

    default:
        break;
    }
}