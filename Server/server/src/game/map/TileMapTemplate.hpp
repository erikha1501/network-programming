#pragma once

#include "../../core/constant.hpp"
#include "Coord2D.hpp"
#include "TileType.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace game::map
{

class TileMapTemplate
{
public:
    constexpr static uint8_t s_tileTypeMask = 0b00000011;
    constexpr static uint8_t s_tileDetailsMaskShift = 2;

public:
    TileMapTemplate() : m_width(0), m_height(0), m_playerSpawnPositions{}, m_tiles{}
    {
    }

    TileMapTemplate(int width, int height)
        : m_width(width), m_height(height), m_playerSpawnPositions{}, m_tiles(width * height)
    {
    }

    TileMapTemplate& operator=(const TileMapTemplate&) = delete;

    TileMapTemplate(TileMapTemplate&&) = default;
    TileMapTemplate& operator=(TileMapTemplate&&) = default;

private:
    TileMapTemplate(const TileMapTemplate&) = default;

public:
    static inline const char* s_defaultMapFileName = "default.bomap";
    static const TileMapTemplate& getDefault();

    static TileMapTemplate loadFromFile(const char* fileName);
    static void saveToFile(const TileMapTemplate& tileMapTemplate, const char* fileName = s_defaultMapFileName);

    int width() const
    {
        return m_width;
    }
    int height() const
    {
        return m_height;
    }

    std::array<Coord2D, g_maxPlayerCount>& playerSpawnPositions()
    {
        return m_playerSpawnPositions;
    }
    const std::array<Coord2D, g_maxPlayerCount>& playerSpawnPositions() const
    {
        return m_playerSpawnPositions;
    }

    uint8_t* data()
    {
        return m_tiles.data();
    }
    const uint8_t* data() const
    {
        return m_tiles.data();
    }

    size_t size() const
    {
        return m_tiles.size();
    }

    TileType atPosition(int x, int y) const
    {
        return (TileType)(m_tiles[x + m_width * y] & s_tileTypeMask);
    }

    TileType atIndex(size_t index) const
    {
        return (TileType)(m_tiles[index] & s_tileTypeMask);
    }

    void clear();

    TileMapTemplate clone() const;

private:
    static TileMapTemplate s_cache;
    static bool s_cacheValid;

private:
    int m_width;
    int m_height;

    std::array<Coord2D, g_maxPlayerCount> m_playerSpawnPositions;

    // |  detail  |tile type|
    // ------6---------2-----
    std::vector<uint8_t> m_tiles;
};

} // namespace game::map
