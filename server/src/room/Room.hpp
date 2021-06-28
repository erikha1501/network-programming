#pragma once

#include "../core/constant.hpp"
#include "../core/IPEndpoint.hpp"
#include "Player.hpp"

#include <pnet.pb.h>

#include <array>

namespace room
{

class Room
{
public:
    struct PlayerEntry
    {
        Player player;
        bool valid;

        PlayerEntry() : player{}, valid(false)
        {
        }

        PlayerEntry(Player player) : player(std::move(player)), valid(true)
        {
        }
    };

    enum class Status : uint8_t
    {
        Idle,
        InGame
    };

public:
    using PlayerList = std::array<PlayerEntry, g_maxPlayerCount>;
    using iterator = PlayerList::iterator;
    using const_iterator = PlayerList::const_iterator;

    Room();

    bool isFull() const
    {
        return m_playerCount >= g_maxPlayerCount;
    }

    int ownerID() const
    {
        return m_ownerID;
    }

    Status status() const
    {
        return m_status;
    }

    void setStatus(Status status)
    {
        m_status = status;
    }

    int addPlayer(Player player);
    bool removePlayer(uint id);

    const Player* findPlayer(uint id) const;
    const Player* findPlayer(const IPEndpoint& endpoint) const;

    const PlayerList& playerList() const
    {
        return m_playerList;
    }

    size_t playerCount() const
    {
        return m_playerCount;
    }

    const_iterator begin() const
    {
        return m_playerList.begin();
    }
    const_iterator end() const
    {
        return m_playerList.end();
    }

    void serializeToRoomInfo(pnet::RoomInfo& roomInfo, uint roomID) const;

private:
    PlayerList m_playerList;
    uint m_playerCount;

    int m_ownerID;

    Status m_status;
};

} // namespace room