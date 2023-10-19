#include "Room.hpp"

namespace room
{

Room::Room() : m_playerList{}, m_playerCount(0), m_ownerID(-1), m_status{}
{
}

int Room::addPlayer(Player player)
{
    if (isFull())
    {
        return -1;
    }

    // Find the first free slot
    for (size_t i = 0; i < m_playerList.size(); i++)
    {
        if (!m_playerList[i].valid)
        {
            m_playerList[i] = PlayerEntry{std::move(player)};
            m_playerCount++;

            // Assign the first player entering the room as owner
            if (m_playerCount == 1)
            {
                m_ownerID = i;
            }

            return i;
        }
    }

    // Should not reach here
    return -1;
}

bool Room::removePlayer(uint id)
{
    if (id >= m_playerList.size())
    {
        return false;
    }

    if (!m_playerList[id].valid)
    {
        return false;
    }

    // Remove player by resetting entry slot
    m_playerList[id] = PlayerEntry{};
    m_playerCount--;

    // Owner leave the room and there's at least one player left
    if (id == m_ownerID && m_playerCount > 0)
    {
        // Find the first valid entry and assign it as the owner
        for (size_t i = 0; i < m_playerList.size(); i++)
        {
            if (m_playerList[i].valid)
            {
                m_ownerID = i;
                break;
            }
        }
    }

    return true;
}

const Player* Room::findPlayer(uint id) const
{
    if (id >= m_playerList.size())
    {
        return nullptr;
    }

    if (!m_playerList[id].valid)
    {
        return nullptr;
    }

    return &(m_playerList[id].player);
}

const Player* Room::findPlayer(const IPEndpoint& endpoint) const
{
    for (const auto& playerEntry : m_playerList)
    {
        if (playerEntry.valid && playerEntry.player.endpoint() == endpoint)
        {
            return &(playerEntry.player);
        }
    }

    return nullptr;
}

void Room::serializeToRoomInfo(pnet::RoomInfo& roomInfo, uint roomID) const
{
    roomInfo.set_id(roomID);
    roomInfo.set_is_full(isFull());
    roomInfo.set_owner_id(m_ownerID);

    switch (m_status)
    {
    case Status::Idle:
        roomInfo.set_status(pnet::RoomInfo_Status_Idle);
        break;
    case Status::InGame:
        roomInfo.set_status(pnet::RoomInfo_Status_InGame);
    default:
        break;
    }

    for (size_t i = 0; i < m_playerList.size(); i++)
    {
        const PlayerEntry& playerEntry = m_playerList[i];
        if (playerEntry.valid)
        {
            pnet::ClientInfo* clientInfo = roomInfo.add_clients();
            clientInfo->set_name(playerEntry.player.name());
            clientInfo->set_id(i);
        }
    }
}

} // namespace room