#pragma once

#include "Room.hpp"

#include "../core/constant.hpp"
#include "../core/UDPSocket.hpp"
#include "../util/ReusableIdList.hpp"

#include "MessageType.hpp"

#include "GameServersManager.hpp"

#include <pnet.pb.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace room
{

class RoomServer
{
private:
struct PlayerIdentity
{
    uint roomID;
    uint playerID;
    bool isConnectionValid;

    PlayerIdentity(uint roomID, uint playerID) : isConnectionValid(true)
    {
        this->roomID = roomID;
        this->playerID = playerID;
    }
};


public:
    RoomServer(uint16_t port = 6969, int maxRoomCount = 4, size_t messageBufferSize = 1024);

    void start();
    void stop();
    void wait();

private:
    using PlayerIdentityMap_t = std::unordered_map<IPEndpoint, PlayerIdentity, IPEndpoint::Hasher>;

private:
    void startInternal();
    void pollPlayerMessages();

    // Send ConnectionPing requests to players in each room
    void performRoomFunctionalCheck();

    void handleMessage(const IPEndpoint& sender, const uint8_t* buffer, size_t size);

    void sendMessage(const IPEndpoint& receiver, pnet::MessageType type, const google::protobuf::Message& message);

    bool validatePlayerIdentity(const pnet::ClientIdentity& identity, const IPEndpoint& endpoint) const;

    void onConnectionPong(const IPEndpoint& sender, const pnet::ConnectionPong& connectionPong);
    void onServerConnectionPing(const IPEndpoint& sender, const pnet::ServerConnectionPing& connectionPong);
    void onQueryRoomInfo(const IPEndpoint& sender, const pnet::QueryRoomInfoRequest& request);
    void onCreateRoom(const IPEndpoint& sender, const pnet::CreateRoomRequest& request);
    void onJoinRoom(const IPEndpoint& sender, const pnet::JoinRoomRequest& request);
    void onLeaveRoom(const IPEndpoint& sender, const pnet::LeaveRoomRequest& request);
    void onQuerySelfRoomInfo(const IPEndpoint& sender, const pnet::QuerySelfRoomInfoRequest& request);
    void onStartGame(const IPEndpoint& sender, const pnet::StartGameRequest& request);

    // Return player's assigned ID or -1 if failed
    int addPlayerToRoom(Room& room, uint roomID, Player player);

    // Return the requested room (if it still exists)
    Room* removePlayerFromRoom(uint roomID, uint playerID);

    void broadcastMessageInRoom(pnet::MessageType messageType, const google::protobuf::Message& message, const Room& room);

    void changeRoomStatus(uint roomID, room::Room::Status status);
    bool isRoomInGame(uint roomID);

private:
    util::ReusableIdList<Room> m_roomList;
    PlayerIdentityMap_t m_playerIdentityMap;

    GameServersManager m_gameServersManager;

    std::unordered_set<uint> m_inGameRoomIDSet;
    std::mutex m_inGameRoomIDSetMutex;

    size_t m_bufferSize;
    std::unique_ptr<uint8_t[]> m_buffer;

    bool m_running;

    UDPSocket m_socket;
};

} // namespace room