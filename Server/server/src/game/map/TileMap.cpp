#include "TileMap.hpp"

#include <cassert>
#include <cstring>
#include <fstream>

namespace game::map
{

TileMap::TileMap(const TileMapTemplate& tileMapTemplate)
{
    m_width = tileMapTemplate.width();
    m_height = tileMapTemplate.height();

    m_tiles = std::vector<Tile>(m_width * m_height);

    // Populate TileMap with TileType information
    for (size_t i = 0; i < m_tiles.size(); i++)
    {
        m_tiles[i] = Tile{tileMapTemplate.atIndex(i)};
    }

    // Copy player spawn positions
    m_playerSpawnPositions = tileMapTemplate.playerSpawnPositions();
}

void TileMap::clear()
{
    std::fill(m_tiles.begin(), m_tiles.end(), Tile{});
}

void TileMap::clearPlayerPresence()
{
    for (auto& tile : m_tiles)
    {
        tile.clearPlayerPresence();
    }
}

void TileMap::clearEntityPresence()
{
    for (auto& tile : m_tiles)
    {
        tile.clearAllPresence();
    }
}

} // namespace game::map
