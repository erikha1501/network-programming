#pragma once

#include "../core/IPEndpoint.hpp"
#include "../core/constant.hpp"

namespace game
{

class PlayerInfo
{
public:
    enum class ConnectionState
    {
        Invalid = 0,
        Disconnected = 1,
        Connecting = 2,
        Connected = 3
    };

public:
    PlayerInfo() : m_heartbeatTimer{}, m_connectionState{}, m_endpoint{}, m_messageSequenceNumber(0)
    {
    }

    PlayerInfo(IPEndpoint endpoint)
        : m_heartbeatTimer{}, m_connectionState(ConnectionState::Disconnected),
          m_endpoint(std::move(endpoint)), m_messageSequenceNumber(0)
    {
    }

    bool isValid() const
    {
        return m_connectionState != ConnectionState::Invalid;
    }

    ConnectionState connectionState() const
    {
        return m_connectionState;
    }
    void setConnectionState(ConnectionState connectionState)
    {
        m_connectionState = connectionState;
    }

    const IPEndpoint& endpoint() const
    {
        return m_endpoint;
    }

    uint messageSequenceNumber() const
    {
        return m_messageSequenceNumber;
    }
    void updateMessageSequenceNumber(uint sequenceNumber)
    {
        m_messageSequenceNumber = sequenceNumber;
    }

    void setEndpoint(IPEndpoint endpoint)
    {
        m_endpoint = std::move(endpoint);
    }

    void tickHeartbeatTimer(time_duration deltaTime)
    {
        m_heartbeatTimer -= deltaTime;
    }

    void resetHeartbeatTimer()
    {
        m_heartbeatTimer = g_playerHeartbeatTimeout;
    }

    bool hasHeartbeatTimedOut() const
    {
        return m_heartbeatTimer <= time_duration{0.0f};
    }

private:
    time_duration m_heartbeatTimer;
    ConnectionState m_connectionState;
    IPEndpoint m_endpoint;

    uint m_messageSequenceNumber;
};

} // namespace game
