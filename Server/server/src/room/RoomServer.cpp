#include "RoomServer.hpp"

#include <stdio.h>
#include <unordered_set>

namespace room
{

RoomServer::RoomServer(uint16_t port, int maxRoomCount, size_t messageBufferSize)
    : m_roomList(maxRoomCount), m_playerIdentityMap{},
      m_gameServersManager(maxRoomCount), m_inGameRoomIDSet{}, m_inGameRoomIDSetMutex{},
      m_bufferSize(messageBufferSize), m_buffer(new uint8_t[messageBufferSize]), m_running(false), m_socket(port, false)
{
}

void RoomServer::start()
{
    m_gameServersManager.registerGameServerEndedCallback([this](const game::GameServer& gameServer) {
        this->changeRoomStatus(gameServer.roomID(), room::Room::Status::Idle);
    });

    m_gameServersManager.start();

    m_running = true;
    startInternal();
}
void RoomServer::stop()
{
    m_running = false;
    m_gameServersManager.stop();
}
void RoomServer::wait()
{
    m_gameServersManager.wait();
}

void RoomServer::startInternal()
{
    constexpr time_duration sleepTime{0.5f};
    time_duration roomFunctionalCheckTimer{};
    hrc_time_point currentTime = hrclock::now();

    while (m_running)
    {
        // Calculate time between frames
        hrc_time_point newTime = hrclock::now();
        time_duration frameTime = std::chrono::duration_cast<time_duration>(newTime - currentTime);
        currentTime = newTime;

        pollPlayerMessages();

        if (roomFunctionalCheckTimer >= room::g_playerHeartbeatTimeout)
        {
            performRoomFunctionalCheck();
            roomFunctionalCheckTimer = time_duration{0.0f};
        }

        roomFunctionalCheckTimer += frameTime;

        time_duration usedTime = hrclock::now() - currentTime;
        std::this_thread::sleep_for(sleepTime - usedTime);
    }
}

void RoomServer::pollPlayerMessages()
{
    IPEndpoint sender;
    while (true)
    {
        ssize_t read = m_socket.recv(m_buffer.get(), m_bufferSize, sender);

        if (read == -1)
        {
            break;
        }

        if (read >= m_bufferSize)
        {
            continue;
        }

        printf("[RoomServer] Received %ld byte packet: %s:%d\n", read, sender.addressAsStr(), sender.port());
        handleMessage(sender, m_buffer.get(), read);
    }
}

void RoomServer::performRoomFunctionalCheck()
{
    pnet::ConnectionPing connectionPing;

    auto iterator = m_playerIdentityMap.begin();
    while (iterator != m_playerIdentityMap.end())
    {
        PlayerIdentity& playerIdentity = iterator->second;
        uint roomID = playerIdentity.roomID;
        uint playerID = playerIdentity.playerID;

        // Skip players that are playing
        if (isRoomInGame(roomID))
        {
            ++iterator;
            continue;
        }

        if (playerIdentity.isConnectionValid)
        {
            playerIdentity.isConnectionValid = false;

            // Send ConnectionPing message
            sendMessage(iterator->first, pnet::MessageType::ConnectionPing, connectionPing);

            // Move to next entry
            ++iterator;
        }
        else
        {
            // Remove player from room
            Room* room = removePlayerFromRoom(roomID, playerID);

            // Remove entry
            iterator = m_playerIdentityMap.erase(iterator);

            // Broadcast RoomInfoChanged message if Room still exists
            if (room != nullptr)
            {
                pnet::RoomInfoChanged roomInfoChanged;
                room->serializeToRoomInfo(*roomInfoChanged.mutable_new_room_info(), roomID);

                for (const auto& playerEntry : *room)
                {
                    if (playerEntry.valid)
                    {
                        sendMessage(playerEntry.player.endpoint(), pnet::MessageType::RoomInfoChanged,
                                    roomInfoChanged);
                    }
                }
            }
        }
    }
}

void RoomServer::sendMessage(const IPEndpoint& receiver, pnet::MessageType type,
                             const google::protobuf::Message& message)
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

void RoomServer::handleMessage(const IPEndpoint& sender, const uint8_t* buffer, size_t size)
{
    // Read message type
    pnet::MessageType mesgType = (pnet::MessageType)buffer[0];
    size_t messageSize = size - 1;
    buffer += 1;

    switch (mesgType)
    {
    case pnet::MessageType::ConnectionPong: {
        pnet::ConnectionPong connectionPong;
        bool parseResult = connectionPong.ParseFromArray(buffer, messageSize);
        assert(parseResult);
        onConnectionPong(sender, connectionPong);
        break;
    }
    case pnet::MessageType::ServerConnectionPing: {
        pnet::ServerConnectionPing serverConnectionPing;
        bool parseResult = serverConnectionPing.ParseFromArray(buffer, messageSize);
        assert(parseResult);
        onServerConnectionPing(sender, serverConnectionPing);
        break;
    }
    case pnet::MessageType::QueryRoomInfoRequest: {
        pnet::QueryRoomInfoRequest request;
        bool parseResult = request.ParseFromArray(buffer, messageSize);
        assert(parseResult);
        onQueryRoomInfo(sender, request);
        break;
    }
    case pnet::MessageType::CreateRoomRequest: {
        pnet::CreateRoomRequest request;
        bool parseResult = request.ParseFromArray(buffer, messageSize);
        assert(parseResult);
        onCreateRoom(sender, request);
        break;
    }
    case pnet::MessageType::JoinRoomRequest: {
        pnet::JoinRoomRequest request;
        bool parseResult = request.ParseFromArray(buffer, messageSize);
        assert(parseResult);
        onJoinRoom(sender, request);
        break;
    }
    case pnet::MessageType::LeaveRoomRequest: {
        pnet::LeaveRoomRequest request;
        bool parseResult = request.ParseFromArray(buffer, messageSize);
        assert(parseResult);
        onLeaveRoom(sender, request);
        break;
    }
    case pnet::MessageType::QuerySelfRoomInfoRequest: {
        pnet::QuerySelfRoomInfoRequest request;
        bool parseResult = request.ParseFromArray(buffer, messageSize);
        assert(parseResult);
        onQuerySelfRoomInfo(sender, request);
        break;
    }

    case pnet::MessageType::StartGameRequest: {
        pnet::StartGameRequest request;
        bool parseResult = request.ParseFromArray(buffer, messageSize);
        assert(parseResult);
        onStartGame(sender, request);
        break;
    }
    default:
        break;
    }
}

bool RoomServer::validatePlayerIdentity(const pnet::ClientIdentity& identity, const IPEndpoint& endpoint) const
{
    const auto iterator = m_playerIdentityMap.find(endpoint);
    if (iterator == m_playerIdentityMap.end())
    {
        return false;
    }
    const PlayerIdentity& playerEntry = iterator->second;
    return playerEntry.playerID == identity.client_id() && playerEntry.roomID == identity.room_id();
}

void RoomServer::onConnectionPong(const IPEndpoint& sender, const pnet::ConnectionPong& connectionPong)
{
    if (connectionPong.has_client_identity())
    {
        const auto iterator = m_playerIdentityMap.find(sender);
        if (iterator == m_playerIdentityMap.end())
        {
            return;
        }
        const pnet::ClientIdentity& identity = connectionPong.client_identity();
        PlayerIdentity& playerIdentity = iterator->second;
        if (playerIdentity.playerID == identity.client_id() && playerIdentity.roomID == identity.room_id())
        {
            playerIdentity.isConnectionValid = true;
        }
    }
}

void RoomServer::onServerConnectionPing(const IPEndpoint& sender, const pnet::ServerConnectionPing& connectionPong)
{
    pnet::ServerConnectionPong serverConnectionPong;
    sendMessage(sender, pnet::MessageType::ServerConnectionPong, serverConnectionPong);
}

void RoomServer::onQueryRoomInfo(const IPEndpoint& sender, const pnet::QueryRoomInfoRequest& request)
{
    pnet::QueryRoomInfoResponse response;

    for (const auto& [id, room] : m_roomList)
    {
        if (request.not_full() && room.isFull())
        {
            continue;
        }

        room.serializeToRoomInfo(*response.add_rooms(), id);
    }

    sendMessage(sender, pnet::MessageType::QueryRoomInfoResponse, response);
}

void RoomServer::onCreateRoom(const IPEndpoint& sender, const pnet::CreateRoomRequest& request)
{
    pnet::CreateRoomResponse createRoomResponse;

    // Check if player is already in a room.
    if (m_playerIdentityMap.find(sender) != m_playerIdentityMap.end())
    {
        // TODO: Use specialized error
        createRoomResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::CreateRoomResponse, createRoomResponse);
        return;
    }

    const auto [roomIter, addRoomResult] = m_roomList.add(Room());

    // Fail to create new room
    if (!addRoomResult)
    {
        createRoomResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::CreateRoomResponse, createRoomResponse);
        return;
    }

    auto& [roomID, newRoom] = *roomIter;
    printf("[RoomServer] Room #%d created\n", roomID);

    Player player{request.client_info().name(), sender};
    int playerID = addPlayerToRoom(newRoom, roomID, std::move(player));

    // Fail to add player
    if (playerID < 0)
    {
        createRoomResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::CreateRoomResponse, createRoomResponse);
        return;
    }

    // Insert new player entry into entry map
    m_playerIdentityMap.emplace(sender, PlayerIdentity{roomID, (uint)playerID});


    // Send response
    createRoomResponse.set_success(true);
    pnet::ClientIdentity* clientIdentity = createRoomResponse.mutable_assigned_identity();
    clientIdentity->set_room_id(roomID);
    clientIdentity->set_client_id(playerID);

    newRoom.serializeToRoomInfo(*createRoomResponse.mutable_room_info(), roomID);

    sendMessage(sender, pnet::MessageType::CreateRoomResponse, createRoomResponse);

    // // Broadcast RoomInfoChanged message
    // pnet::RoomInfoChanged roomInfoChanged;
    // newRoom.serializeToRoomInfo(*roomInfoChanged.mutable_new_room_info());

    // broadcastMessageInRoom(pnet::MessageType::RoomInfoChanged, roomInfoChanged, newRoom);
}

void RoomServer::onJoinRoom(const IPEndpoint& sender, const pnet::JoinRoomRequest& request)
{
    pnet::JoinRoomResponse joinRoomResponse;

    // Check if player is already in a room.
    if (m_playerIdentityMap.find(sender) != m_playerIdentityMap.end())
    {
        // TODO: Use specialized error
        joinRoomResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::JoinRoomResponse, joinRoomResponse);
        return;
    }

    uint roomID = request.room_id();
    Room* room = m_roomList.get(roomID);

    // Cannot find room with the given ID.
    if (room == nullptr)
    {
        joinRoomResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::JoinRoomResponse, joinRoomResponse);
        return;
    }

    // Try to add player to the room
    Player player{request.client_info().name(), sender};
    int playerID = addPlayerToRoom(*room, roomID, std::move(player));

    // Fail to add player
    if (playerID < 0)
    {
        joinRoomResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::JoinRoomResponse, joinRoomResponse);
        return;
    }

    // Insert new player entry into entry map
    m_playerIdentityMap.emplace(sender, PlayerIdentity{roomID, (uint)playerID});

    // Send response
    joinRoomResponse.set_success(true);
    pnet::ClientIdentity* clientIdentity = joinRoomResponse.mutable_assigned_identity();
    clientIdentity->set_room_id(roomID);
    clientIdentity->set_client_id(playerID);

    room->serializeToRoomInfo(*joinRoomResponse.mutable_room_info(), roomID);

    sendMessage(sender, pnet::MessageType::JoinRoomResponse, joinRoomResponse);

    // Broadcast RoomInfoChanged message
    pnet::RoomInfoChanged roomInfoChanged;
    room->serializeToRoomInfo(*roomInfoChanged.mutable_new_room_info(), roomID);

    broadcastMessageInRoom(pnet::MessageType::RoomInfoChanged, roomInfoChanged, *room);
}

void RoomServer::onLeaveRoom(const IPEndpoint& sender, const pnet::LeaveRoomRequest& request)
{
    pnet::LeaveRoomResponse leaveRoomResponse;

    if (!request.has_client_identity() || !validatePlayerIdentity(request.client_identity(), sender))
    {
        leaveRoomResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::LeaveRoomResponse, leaveRoomResponse);
        return;
    }

    uint roomID = request.client_identity().room_id();
    uint clientID = request.client_identity().client_id();

    Room* room = removePlayerFromRoom(roomID, clientID);

    // Remove player entry
    m_playerIdentityMap.erase(sender);

    // Send response
    leaveRoomResponse.set_success(true);
    sendMessage(sender, pnet::MessageType::LeaveRoomResponse, leaveRoomResponse);

    // Broadcast RoomInfoChanged message if Room still exists
    if (room != nullptr)
    {
        pnet::RoomInfoChanged roomInfoChanged;
        room->serializeToRoomInfo(*roomInfoChanged.mutable_new_room_info(), roomID);

        broadcastMessageInRoom(pnet::MessageType::RoomInfoChanged, roomInfoChanged, *room);
    }
}

void RoomServer::onQuerySelfRoomInfo(const IPEndpoint& sender, const pnet::QuerySelfRoomInfoRequest& request)
{
    if (!request.has_client_identity() || !validatePlayerIdentity(request.client_identity(), sender))
    {
        return;
    }

    uint roomID = request.client_identity().room_id();
    const Room* room = m_roomList.get(roomID);

    pnet::RoomInfoChanged roomInfoChanged;
    room->serializeToRoomInfo(*roomInfoChanged.mutable_new_room_info(), roomID);

    sendMessage(sender, pnet::MessageType::RoomInfoChanged, roomInfoChanged);
}

void RoomServer::onStartGame(const IPEndpoint& sender, const pnet::StartGameRequest& request)
{
    pnet::StartGameResponse startGameResponse;

    if (!request.has_client_identity() || !validatePlayerIdentity(request.client_identity(), sender))
    {
        // TODO: Use specialized error
        startGameResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::StartGameResponse, startGameResponse);
        return;
    }

    uint roomID = request.client_identity().room_id();
    uint clientID = request.client_identity().client_id();

    // Check if requester is the room owner
    const Room* room = m_roomList.get(roomID);
    if (clientID != room->ownerID())
    {
        // TODO: Use specialized error
        startGameResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::StartGameResponse, startGameResponse);
        return;
    }

    // Check if room has enough players
    if (room->playerCount() < g_minPlayerCount)
    {
        // TODO: Use specialized error
        startGameResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::StartGameResponse, startGameResponse);
        return;
    }

    // Check for game server limit
    if (m_gameServersManager.isFull())
    {
        // TODO: Use specialized error
        startGameResponse.set_success(false);
        sendMessage(sender, pnet::MessageType::StartGameResponse, startGameResponse);
        return;
    }

    // Reset player connection validation flags
    // This ensures players won't get kicked out of the room
    // right after they exit a game
    for (const auto& playerEntry : *room)
    {
        if (!playerEntry.valid)
        {
            continue;
        }

        auto& [_, playerIdentity] = *m_playerIdentityMap.find(playerEntry.player.endpoint());
        playerIdentity.isConnectionValid = true;
    }
    

    // Create new game server
    std::unique_ptr<game::GameServer> gameServer = std::make_unique<game::GameServer>(roomID, 0);

    const Room::PlayerList& playerList = room->playerList();
    for (size_t i = 0; i < playerList.size(); i++)
    {
        if (playerList[i].valid)
        {
            gameServer->registerPlayer(i, playerList[i].player.endpoint());
        }
    }

    uint16_t gameServerPort = gameServer->port();

    assert(m_gameServersManager.dispatchGameServer(std::move(gameServer)));

    // Update room status
    changeRoomStatus(roomID, room::Room::Status::InGame);

    startGameResponse.set_success(true);
    sendMessage(sender, pnet::MessageType::StartGameResponse, startGameResponse);

    // Broadcast GameCreated message
    pnet::GameCreated gameCreated;
    gameCreated.set_port(gameServerPort);
    broadcastMessageInRoom(pnet::MessageType::GameCreated, gameCreated, *room);
}

int RoomServer::addPlayerToRoom(Room& room, uint roomID, Player player)
{
    int playerID = room.addPlayer(std::move(player));

    if (playerID >= 0)
    {
        printf("[RoomServer] Room #%d adds player #%d\n", roomID, playerID);
    }
    
    return playerID;
}

Room* RoomServer::removePlayerFromRoom(uint roomID, uint playerID)
{
    Room* room = m_roomList.get(roomID);
    room->removePlayer(playerID);

    printf("[RoomServer] Room #%d removes player #%d\n", roomID, playerID);

    // The last player has left the room
    if (room->playerCount() == 0)
    {
        m_roomList.remove(roomID);
        room = nullptr;

        printf("[RoomServer] Room #%d removed\n", roomID);
    }

    return room;
}

void RoomServer::broadcastMessageInRoom(pnet::MessageType messageType, const google::protobuf::Message& message,
                                        const Room& room)
{
    for (const auto& playerEntry : room)
    {
        if (playerEntry.valid)
        {
            sendMessage(playerEntry.player.endpoint(), messageType, message);
        }
    }
}

void RoomServer::changeRoomStatus(uint roomID, room::Room::Status status)
{
    {
        std::scoped_lock lock{m_inGameRoomIDSetMutex};

        if (status == room::Room::Status::InGame)
        {
            m_inGameRoomIDSet.emplace(roomID);
        }
        else if (status == room::Room::Status::Idle)
        {
            m_inGameRoomIDSet.erase(roomID);
        }
    }

    m_roomList.get(roomID)->setStatus(status);

    printf("[RoomServer] Room #%d changes status\n", roomID);
}

bool RoomServer::isRoomInGame(uint roomID)
{
    std::scoped_lock lock{m_inGameRoomIDSetMutex};
    return m_inGameRoomIDSet.find(roomID) != m_inGameRoomIDSet.end();
}

} // namespace room