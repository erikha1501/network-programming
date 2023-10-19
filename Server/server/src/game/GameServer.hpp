#pragma once

#include "../core/constant.hpp"

#include "../core/UDPSocket.hpp"

#include "GameWorld.hpp"
#include "MessageType.hpp"
#include "PlayerInfo.hpp"

#include "../util/ActionCallback.hpp"

#include <pgame.pb.h>

#include <array>
#include <thread>

namespace game
{

class GameServer
{
public:
    enum class State : uint8_t
    {
        Stopped,
        WaitingForPlayer,
        WaitingForCountdown,
        Playing
    };

    enum class ErrorCode : uint8_t
    {
        None,
        InsufficientPlayer,
        AllPlayersDisconnected,
        Internal
    };

public:
    GameServer(uint roomID, uint16_t port);

    bool registerPlayer(uint id, const IPEndpoint& endpoint);
    uint registeredPlayerCount() const
    {
        return m_registeredPlayerCount;
    }

    void registerEndCallback(util::ActionCallback* callback)
    {
        m_endCallback = callback;
    }

    uint gameServerID() const
    {
        return m_serverID;
    }
    void setServerID(uint serverID)
    {
        m_serverID = serverID;
    }

    uint roomID() const
    {
        return m_roomID;
    }

    void start();
    void stop();
    void wait();

    const std::array<PlayerInfo, g_maxPlayerCount>& playerInfoList() const
    {
        return m_playerInfoList;
    }

    uint16_t port() const
    {
        return m_socket.bindedPort();
    }

    State state() const
    {
        return m_state;
    }
    ErrorCode errorCode() const
    {
        return m_errorCode;
    }

private:
    void startInternal();
    void executeStages();

    bool establishConnection();
    void pollPlayerConnectionRequests(uint& connectedPlayerCount);
    void executeCountdownSequence();

    void executeGameLoop();

    void pollPlayerMessages();
    void sendGameState();
    void broadcastPingMessages();

    void checkGameEndCondition();

    void tickPlayerHeartbeatTimers(time_duration deltaTime);

    bool validatePlayerIdentity(const pgame::PlayerIdentity& identity, const IPEndpoint& endpoint) const;
    bool validatePlayerIdentityAnyPort(const pgame::PlayerIdentity& identity, const IPEndpoint& endpoint) const;
    void sendMessage(const IPEndpoint& receiver, pgame::MessageType type, const google::protobuf::Message& message);

    void sendMessageToAllConnectedPlayers(pgame::MessageType type, const google::protobuf::Message& message);

private:
    uint m_serverID;
    uint m_roomID;

    std::array<PlayerInfo, g_maxPlayerCount> m_playerInfoList;
    uint m_registeredPlayerCount;

    GameWorld m_gameWorld;

    time_duration m_gameDurationTimer;

    State m_state;
    ErrorCode m_errorCode;

    static constexpr uint s_bufferSize = 1024;
    std::unique_ptr<uint8_t[]> m_buffer;

    UDPSocket m_socket;
    uint m_serverMessageSequenceNumber;

    util::ActionCallback* m_endCallback;

    std::thread m_thread;
};

} // namespace game
