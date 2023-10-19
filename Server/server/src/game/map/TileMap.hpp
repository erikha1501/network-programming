#pragma once

#include "Tile.hpp"
#include "TileMapTemplate.hpp"

#include <array>
#include <vector>

namespace game::map
{

class TileMap
{
public:
    TileMap() : m_width(0), m_height(0), m_playerSpawnPositions{}, m_tiles{}
    {
    }

    TileMap(const TileMapTemplate& tileMapTemplate);

    TileMap& operator=(const TileMap&) = delete;

    TileMap(TileMap&&) = default;
    TileMap& operator=(TileMap&&) = default;

private:
    TileMap(const TileMap&) = default;

public:
    int width() const
    {
        return m_width;
    }
    int height() const
    {
        return m_height;
    }

    const std::array<Coord2D, g_maxPlayerCount>& playerSpawnPositions() const
    {
        return m_playerSpawnPositions;
    }
    Coord2D playerSpawnPosition(uint playerID) const
    {
        return m_playerSpawnPositions[playerID];
    }

    size_t size() const
    {
        return m_tiles.size();
    }

    Tile& at(int x, int y)
    {
        return m_tiles[x + m_width * y];
    }
    const Tile& at(int x, int y) const
    {
        return m_tiles[x + m_width * y];
    }
    Tile& atIndex(size_t index)
    {
        return m_tiles[index];
    }
    const Tile& atIndex(size_t index) const
    {
        return m_tiles[index];
    }

    void clear();
    void clearPlayerPresence();
    void clearEntityPresence();

private:
    int m_width;
    int m_height;

    std::array<Coord2D, g_maxPlayerCount> m_playerSpawnPositions;
    std::vector<Tile> m_tiles;
};

} // namespace game::map
