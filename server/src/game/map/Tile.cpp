#include "Tile.hpp"

namespace game::map
{

void Tile::clearAllPresence()
{
    m_entityBitSet = 0;
}

void Tile::clearPlayerPresence()
{
    m_entityBitSet &= (~EntityMask::Player);
}

bool Tile::testPresence(uint16_t entityMask) const
{
    return (m_entityBitSet & entityMask) != 0;
}

void Tile::setPresence(uint16_t entityMask)
{
    m_entityBitSet |= entityMask;
}

void Tile::clearPresence(uint16_t entityMask)
{
    m_entityBitSet &= (~entityMask);
}

uint8_t Tile::packAsNetworkData() const
{
    // TODO: Non-empty tile should not reveal powerup information
    return ((m_entityBitSet) << 2) | ((uint8_t)m_type & 0x3);
}

} // namespace game::map
