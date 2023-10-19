#pragma once

#include "../../core/constant.hpp"
#include "TileType.hpp"

#include <array>
#include <cstdint>

namespace game::map
{

struct EntityMask
{
    static constexpr uint16_t Bomb      = 0x0001;
    static constexpr uint16_t Powerup   = 0x0002;

    static constexpr uint16_t Player1   = 0x0100;
    static constexpr uint16_t Player2   = 0x0200;
    static constexpr uint16_t Player3   = 0x0400;
    static constexpr uint16_t Player4   = 0x0800;

    static constexpr uint16_t Player = Player1 | Player2 | Player3 | Player4;

    static constexpr uint16_t Entity = ~0;

    static constexpr std::array<uint16_t, g_maxPlayerCount> s_playerIDToMask{Player1, Player2, Player3, Player4};
};

class Tile
{
public:
    explicit Tile(TileType type = TileType::Empty, void* customData = 0) : m_type(type), m_entityBitSet{}, m_customData(customData)
    {
    }

    TileType type() const
    {
        return m_type;
    }
    void setType(TileType type)
    {
        m_type = type;
    }
    void* customData() const
    {
        return m_customData;
    }
    void setCustomData(void* customData)
    {
        m_customData = customData;
    }

    void clearAllPresence();
    void clearPlayerPresence();

    bool testPresence(uint16_t entityMask) const;
    void setPresence(uint16_t entityMask);
    void clearPresence(uint16_t entityMask);

    // |other|bomb|tile type|
    // ---5----1-------2-----
    uint8_t packAsNetworkData() const;

private:
    TileType m_type;

    // Player entity: Upper 8 bits
    // Other entities: Lower 8 bits
    uint16_t m_entityBitSet;

    // For now, this field is only useful for Breakable tiles
    void* m_customData;
};

} // namespace game::map
