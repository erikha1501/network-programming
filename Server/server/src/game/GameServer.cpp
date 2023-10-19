#include "GameServer.hpp"

#include "MessageType.hpp"

#include "map/TileMapTemplate.hpp"

namespace game
{

// Prototypes
void serializeTileMap(const map::TileMap& mapData, pgame::MapInfo& mapInfo);

GameServer::GameServer(uint roomID, uint16_t port)
    : m_serverID(-1), m_roomID(roomID), m_playerInfoList{},
      m_registeredPlayerCount(0), m_gameWorld{}, m_state{}, m_errorCode{}, m_buffer(new uint8_t[s_bufferSize]),
      m_socket(port, false), m_endCallback{}
{
    m_gameDurationTimer = g_gameMaxDuration;
    m_serverMessageSequenceNumber = 0;
}

bool GameServer::registerPlayer(uint id, const IPEndpoint& endpoint)
{
    assert(id < m_playerInfoList.size());

    m_playerInfoList[id] = PlayerInfo(endpoint);
    m_registeredPlayerCount++;

    return true;
}

void GameServer::start()
{
    m_thread = std::thread{&GameServer::startInternal, this};
}
void GameServer::stop()
{
    m_state = State::Stopped;
}

void GameServer::wait()
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void GameServer::startInternal()
{
    struct StartStopGuard
    {
        StartStopGuard(GameServer& gameServer) : gameServer(gameServer)
        {
            // TODO: Move to logger
            printf("[GameServer #%d] Created at port %d\n", gameServer.m_serverID, gameServer.port());
            printf("[GameServer #%d] Expecting %d players\n", gameServer.m_serverID,
                   gameServer.registeredPlayerCount());
        }

        ~StartStopGuard()
        {
            // TODO: Move to logger
            printf("[GameServer #%d] Stopped", gameServer.m_serverID);
            switch (gameServer.errorCode())
            {
            case ErrorCode::None:
                printf("\n");
                break;
            case ErrorCode::InsufficientPlayer:
                printf(": Not enough player connected before timed out\n");
                break;
            case ErrorCode::AllPlayersDisconnected:
                printf(": All players have disconnected\n");
                break;
            case ErrorCode::Internal:
                printf(": Internal error\n");
            default:
                break;
            }

            if (gameServer.m_endCallback != nullptr)
            {
                gameServer.m_endCallback->invoke();
            }
        }
        GameServer& gameServer;
    } guard{*this};

    try
    {
        executeStages();
        m_state = State::Stopped;
    }
    catch (const std::exception& e)
    {
        // TODO: Log
        m_errorCode = ErrorCode::Internal;
    }
}

void GameServer::executeStages()
{
    // Stage 1: Wait for players to connect
    m_state = State::WaitingForPlayer;
    bool result = establishConnection();
    if (!result)
    {
        m_errorCode = ErrorCode::InsufficientPlayer;
        return;
    }

    // Check for early exit
    if (m_state == State::Stopped)
    {
        return;
    }

    // For each connected players:
    // Add to game world
    // Add to list of connected players in GameStatusChanged {Starting} message
    pgame::GameStatusChanged gameStatusChanged;
    gameStatusChanged.set_status(pgame::GameStatusChanged_Status_Starting);
    for (size_t i = 0; i < m_playerInfoList.size(); i++)
    {
        if (m_playerInfoList[i].connectionState() == PlayerInfo::ConnectionState::Connected)
        {
            m_gameWorld.addPlayer(i);
            gameStatusChanged.mutable_starting_info()->add_connected_player_ids(i);
        }
    }

    // Send GameStatusChanged {Starting} message
    sendMessageToAllConnectedPlayers(pgame::MessageType::GameStatusChanged, gameStatusChanged);

    m_gameWorld.initialize();

    // Stage 2: Execute countdown sequence
    m_state = State::WaitingForCountdown;
    executeCountdownSequence();

    // Check for early exit
    if (m_state == State::Stopped)
    {
        return;
    }

    // Reset player heatbeat timers
    for (auto& playerInfo : m_playerInfoList)
    {
        if (playerInfo.connectionState() == PlayerInfo::ConnectionState::Connected)
        {
            playerInfo.resetHeartbeatTimer();
        }
    }

    // Stage 3: Run the game
    printf("[GameServer #%d] Game started\n", m_serverID);
    m_state = State::Playing;
    executeGameLoop();

    // Collect game result
    gameStatusChanged.Clear();
    if (m_errorCode == ErrorCode::None)
    {
        gameStatusChanged.set_status(pgame::GameStatusChanged_Status_Ended);

         // Get alive player ids
        for (uint winnerID : m_gameWorld.getAlivePlayerIDs())
        {
            gameStatusChanged.mutable_end_info()->add_winner_ids(winnerID);
        }
    }
    else
    {
        gameStatusChanged.set_status(pgame::GameStatusChanged_Status_Error);
    }

    // Send GameStatusChanged {Ended}
    sendMessageToAllConnectedPlayers(pgame::MessageType::GameStatusChanged, gameStatusChanged);
}

bool GameServer::establishConnection()
{
    time_duration connectionWaitTimer = g_playerConnectionWaitTimeout;
    constexpr time_duration sleepDuration = time_duration{0.5f};

    uint connectedPlayerCount = 0;

    while (m_state == State::WaitingForPlayer)
    {
        hrc_time_point currentTime = hrclock::now();

        pollPlayerConnectionRequests(connectedPlayerCount);

        // All players have connected, stop waiting
        if (connectedPlayerCount >= m_registeredPlayerCount)
        {
            break;
        }

        // Update timer
        connectionWaitTimer -= sleepDuration;

        time_duration usedTime = hrclock::now() - currentTime;
        std::this_thread::sleep_for(sleepDuration - usedTime);

        if (connectionWaitTimer <= time_duration{0.0f})
        {
            break;
        }
    }

    return connectedPlayerCount >= g_minPlayerCount;
}

void GameServer::pollPlayerConnectionRequests(uint& connectedPlayerCount)
{
    IPEndpoint sender;
    while (true)
    {
        uint8_t* buffer = m_buffer.get();
        int read = m_socket.recv(buffer, s_bufferSize, sender);
        if (read == -1)
        {
            return;
        }

        pgame::MessageType messageType = (pgame::MessageType)buffer[0];
        buffer += 1;
        size_t messageSize = read - 1;

        switch (messageType)
        {
        case pgame::MessageType::JoinGameRequest: {
            pgame::JoinGameRequest joinGameRequest;
            joinGameRequest.ParseFromArray(buffer, messageSize);

            if (!joinGameRequest.has_player_identity() ||
                !validatePlayerIdentityAnyPort(joinGameRequest.player_identity(), sender))
            {
                // TODO: Log
                break;
            }

            uint playerID = joinGameRequest.player_identity().player_id();
            PlayerInfo& playerInfo = m_playerInfoList[playerID];

            pgame::JoinGameResponse joinGameResponse;

            if (playerInfo.connectionState() == PlayerInfo::ConnectionState::Disconnected)
            {
                printf("[GameServer #%d] Player (%d -> %s:%d) requests to join\n", m_serverID, playerID,
                       sender.addressAsStr(), sender.port());
                playerInfo.setConnectionState(PlayerInfo::ConnectionState::Connecting);

                // Update client port
                playerInfo.setEndpoint(sender);

                joinGameResponse.set_success(true);

                pgame::MapInfo& mapInfo = *joinGameResponse.mutable_map_info();
                serializeTileMap(m_gameWorld.tileMap(), mapInfo);
            }
            else
            {
                joinGameResponse.set_success(false);
            }

            sendMessage(sender, pgame::MessageType::JoinGameResponse, joinGameResponse);
            break;
        }
        case pgame::MessageType::MapInfoReceived: {
            pgame::MapInfoReceived mapInfoReceived;
            mapInfoReceived.ParseFromArray(buffer, messageSize);

            if (!mapInfoReceived.has_player_identity() ||
                !validatePlayerIdentity(mapInfoReceived.player_identity(), sender))
            {
                // TODO: Log
                break;
            }

            uint playerID = mapInfoReceived.player_identity().player_id();
            PlayerInfo& playerInfo = m_playerInfoList[playerID];

            if (playerInfo.connectionState() == PlayerInfo::ConnectionState::Connecting)
            {
                printf("[GameServer #%d] Player (%d -> %s:%d) joined\n", m_serverID, playerID, sender.addressAsStr(),
                       sender.port());
                playerInfo.setConnectionState(PlayerInfo::ConnectionState::Connected);
                connectedPlayerCount++;
            }

            break;
        }
        default:
            break;
        }
    }
}

void GameServer::executeCountdownSequence()
{
    time_duration countdownTimer = g_gameStartingCountdownInterval;
    constexpr time_duration sleepDuration = time_duration{0.5f};

    // Periodically send GameStatusChanged {CountingDown} message to all connected players
    pgame::GameStatusChanged gameStatusChanged;

    while (m_state == State::WaitingForCountdown)
    {
        hrc_time_point currentTime = hrclock::now();

        gameStatusChanged.set_status(pgame::GameStatusChanged_Status_CountingDown);
        gameStatusChanged.mutable_countingdown_info()->set_countdown_value(countdownTimer.count());

        sendMessageToAllConnectedPlayers(pgame::MessageType::GameStatusChanged, gameStatusChanged);

        // Update timer
        countdownTimer -= sleepDuration;

        time_duration usedTime = hrclock::now() - currentTime;
        std::this_thread::sleep_for(sleepDuration - usedTime);

        if (countdownTimer <= time_duration{0.0f})
        {
            break;
        }
    }

    // Finish counting down with GameStatusChanged {Started} message
    gameStatusChanged.Clear();
    gameStatusChanged.set_status(pgame::GameStatusChanged_Status_Started);

    sendMessageToAllConnectedPlayers(pgame::MessageType::GameStatusChanged, gameStatusChanged);
}

void GameServer::executeGameLoop()
{
    constexpr time_duration frameDeltaTime{1.0f / 40.0f};   // 40 fps
    constexpr time_duration physicDeltaTime{1.0f / 120.0f}; // 120 tps
    hrc_time_point currentTime = hrclock::now();

    time_duration physicTimeBudget{0.0f};

    while (m_state == State::Playing)
    {
        // Calculate time between frames
        hrc_time_point newTime = hrclock::now();
        time_duration frameTime = std::chrono::duration_cast<time_duration>(newTime - currentTime);
        currentTime = newTime;
        physicTimeBudget += frameTime;

        pollPlayerMessages();

        // Update player input
        m_gameWorld.applyPlayerInput();

        // Physic update
        while (physicTimeBudget >= physicDeltaTime)
        {
            m_gameWorld.physicStep(physicDeltaTime);
            physicTimeBudget -= physicDeltaTime;
        }

        // Game logic update
        m_gameWorld.gameLogicStep(frameTime);

        sendGameState();
        broadcastPingMessages();

        // Heartbeat update
        tickPlayerHeartbeatTimers(frameTime);

        // Game timer update
        m_gameDurationTimer -= frameTime;

        // Check game end
        checkGameEndCondition();

        time_duration usedTime = hrclock::now() - currentTime;
        std::this_thread::sleep_for(frameDeltaTime - usedTime);
    }
}

void GameServer::pollPlayerMessages()
{
    IPEndpoint sender;
    while (true)
    {
        uint8_t* buffer = m_buffer.get();
        ssize_t read = m_socket.recv(buffer, s_bufferSize, sender);
        if (read == -1)
        {
            return;
        }

        pgame::MessageType messageType = (pgame::MessageType)buffer[0];
        buffer += 1;
        size_t messageSize = read - 1;

        switch (messageType)
        {
        case pgame::MessageType::PlayerInput: {
            pgame::PlayerInput playerInputMessage;
            playerInputMessage.ParseFromArray(buffer, messageSize);

            if (!playerInputMessage.has_player_identity() ||
                !validatePlayerIdentity(playerInputMessage.player_identity(), sender))
            {
                break;
            }

            uint playerID = playerInputMessage.player_identity().player_id();

            // Check message sequence number
            PlayerInfo& playerInfo = m_playerInfoList[playerID];
            uint incomingSequenceNum = playerInputMessage.sequence_num();

            if (incomingSequenceNum <= playerInfo.messageSequenceNumber())
            {
                break;
            }

            // Update sequence number
            playerInfo.updateMessageSequenceNumber(incomingSequenceNum);


            // Update player input
            PlayerEntity::InputData& playerInput = m_gameWorld.playerInput(playerID);

            // Use the latest direction input
            playerInput.direction.Set(playerInputMessage.direction_x(), playerInputMessage.direction_y());

            // Any bomb placing message will register bombPlaced flag.
            // This flag will be cleared at applyPlayerInput stage.
            if (playerInputMessage.bomb_placed())
            {
                playerInput.bombPlaced = true;
            }

            break;
        }

        case pgame::MessageType::ConnectionPong: {
            pgame::ConnectionPong connectionPong;
            connectionPong.ParseFromArray(buffer, messageSize);

            if (!connectionPong.has_player_identity() ||
                !validatePlayerIdentity(connectionPong.player_identity(), sender))
            {
                break;
            }

            uint playerID = connectionPong.player_identity().player_id();
            // Reset heartbeat timer for this player
            m_playerInfoList[playerID].resetHeartbeatTimer();

            break;
        }

        default:
            break;
        }
    }
}

void GameServer::sendGameState()
{
    pgame::GameState gameState;

    gameState.set_sequence_num(++m_serverMessageSequenceNumber);

    // Collect tile map data
    // TODO: Remove hardcoded value
    uint8_t mapData[512];
    const map::TileMap& tileMap = m_gameWorld.tileMap();
    for (size_t i = 0; i < tileMap.size(); i++)
    {
        mapData[i] = tileMap.atIndex(i).packAsNetworkData();
    }

    gameState.set_map_data(mapData, tileMap.size());

    // Collect game timer value
    gameState.set_game_timer(m_gameDurationTimer.count());

    // Collect player states
    for (uint i = 0; i < m_playerInfoList.size(); i++)
    {
        PlayerInfo& playerInfo = m_playerInfoList[i];
        if (!playerInfo.isValid())
        {
            continue;
        }

        auto* playerState = gameState.add_player_states();
        playerState->set_player_id(i);

        // Connection state
        if (playerInfo.connectionState() == PlayerInfo::ConnectionState::Connected)
        {
            playerState->set_connection_state(pgame::GameState_PlayerState_ConnectionState_CONNECTED);
        }
        else
        {
            playerState->set_connection_state(pgame::GameState_PlayerState_ConnectionState_DISCONNECTED);
        }

        // Entity state
        const PlayerEntity& playerEntity = m_gameWorld.playerEntity(i);

        if (playerEntity.state() == PlayerEntity::State::Alive)
        {
            playerState->set_state(pgame::GameState_PlayerState_State_ALIVE);

            playerState->set_position_x(playerEntity.body()->GetPosition().x);
            playerState->set_position_y(playerEntity.body()->GetPosition().y);
            playerState->set_direction_x(playerEntity.body()->GetLinearVelocity().x);
            playerState->set_direction_y(playerEntity.body()->GetLinearVelocity().y);
        }
        else
        {
            playerState->set_state(pgame::GameState_PlayerState_State_DEAD);
        }
    }

    // Collect bomb states
    for (const auto& bombEntity : m_gameWorld.bombList())
    {
        auto* bombState = gameState.add_bomb_states();
        bombState->set_id(bombEntity.id());
        bombState->set_position_x(bombEntity.positionX());
        bombState->set_position_y(bombEntity.positionY());
        bombState->set_range(bombEntity.range());
    }

    // Collect powerup states
    for (const auto& [position, powerupEntity] : m_gameWorld.powerupList())
    {
        if (!powerupEntity.isActive())
        {
            continue;
        }
        
        auto* powerupState = gameState.add_powerup_states();
        powerupState->set_id(powerupEntity.id());
        powerupState->set_position_x(powerupEntity.positionX());
        powerupState->set_position_y(powerupEntity.positionY());

        switch (powerupEntity.powerupType())
        {
        case PowerupEntity::Type::BombQuantity:
            powerupState->set_powerup_type(pgame::GameState_PowerupState_Type_BOMB_QUANTITY);
            break;
        case PowerupEntity::Type::BombRange:
            powerupState->set_powerup_type(pgame::GameState_PowerupState_Type_BOMB_RANGE);
            break;
        case PowerupEntity::Type::Speedup:
            powerupState->set_powerup_type(pgame::GameState_PowerupState_Type_SPEEDUP);
            break;
        default:
            break;
        }
    }
    

    // Send game state
    sendMessageToAllConnectedPlayers(pgame::MessageType::GameState, gameState);
}

void GameServer::broadcastPingMessages()
{
    pgame::ConnectionPong connectionPing;
    sendMessageToAllConnectedPlayers(pgame::MessageType::ConnectionPing, connectionPing);
}

void GameServer::checkGameEndCondition()
{
    if (m_gameDurationTimer <= time_duration{0.0f})
    {
        m_state = State::Stopped;
        return;
    }

    if (m_gameWorld.checkEndCondition())
    {
        m_state = State::Stopped;
        return;
    }
}

void GameServer::tickPlayerHeartbeatTimers(time_duration deltaTime)
{
    uint connectedPlayerCount = 0;
    for (auto& playerInfo : m_playerInfoList)
    {
        if (playerInfo.connectionState() == PlayerInfo::ConnectionState::Connected)
        {
            playerInfo.tickHeartbeatTimer(deltaTime);

            if (playerInfo.hasHeartbeatTimedOut())
            {
                playerInfo.setConnectionState(PlayerInfo::ConnectionState::Disconnected);
            }
            else
            {
                connectedPlayerCount++;
            }
        }
    }

    if (connectedPlayerCount == 0)
    {
        m_state = State::Stopped;
        m_errorCode = ErrorCode::AllPlayersDisconnected;
    }
}

bool GameServer::validatePlayerIdentity(const pgame::PlayerIdentity& identity, const IPEndpoint& endpoint) const
{
    if (identity.room_id() != m_roomID || identity.player_id() >= g_maxPlayerCount)
    {
        return false;
    }
    const PlayerInfo& playerInfo = m_playerInfoList[identity.player_id()];

    return playerInfo.isValid() && playerInfo.endpoint() == endpoint;
}

bool GameServer::validatePlayerIdentityAnyPort(const pgame::PlayerIdentity& identity, const IPEndpoint& endpoint) const
{
    if (identity.room_id() != m_roomID || identity.player_id() >= g_maxPlayerCount)
    {
        return false;
    }
    const PlayerInfo& playerInfo = m_playerInfoList[identity.player_id()];

    return playerInfo.isValid() && playerInfo.endpoint().address() == endpoint.address();
}

void GameServer::sendMessage(const IPEndpoint& receiver, pgame::MessageType type,
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

void GameServer::sendMessageToAllConnectedPlayers(pgame::MessageType type, const google::protobuf::Message& message)
{
    for (const auto& playerInfo : m_playerInfoList)
    {
        if (playerInfo.connectionState() != PlayerInfo::ConnectionState::Connected)
        {
            continue;
        }

        sendMessage(playerInfo.endpoint(), type, message);
    }
}

void serializeTileMap(const map::TileMap& tileMap, pgame::MapInfo& mapInfo)
{
    // TODO: GameServer is supposed to choose which map template to use
    // and instructs GameWorld to load that template.
    // When serializing tile map, TileMapTemplate stores detailed information
    // needed by clients.
    mapInfo.set_width(tileMap.width());
    mapInfo.set_height(tileMap.height());

    // Collect tile map data
    // TODO: Remove hardcoded map
    const map::TileMapTemplate& tileMapTemplate = map::TileMapTemplate::getDefault();
    mapInfo.set_map_data(tileMapTemplate.data(), tileMap.size());

    for (uint i = 0; i < g_maxPlayerCount; i++)
    {
        pgame::MapInfo::PlayerTilePosition& spawnPosition = *mapInfo.add_player_spawn_positions();
        spawnPosition.set_player_id(i);
        spawnPosition.set_x(tileMap.playerSpawnPosition(i).x);
        spawnPosition.set_y(tileMap.playerSpawnPosition(i).y);
    }
}

} // namespace game