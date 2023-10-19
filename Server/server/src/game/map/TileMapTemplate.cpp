#include "TileMapTemplate.hpp"

#include <fstream>
#include <cstring>
#include <cassert>

namespace game::map
{

bool TileMapTemplate::s_cacheValid = false;
TileMapTemplate TileMapTemplate::s_cache;

const TileMapTemplate& TileMapTemplate::getDefault()
{
    if (!s_cacheValid)
    {
        s_cache = loadFromFile(s_defaultMapFileName);
        s_cacheValid = true;
    }
    
    return s_cache;
}

TileMapTemplate TileMapTemplate::loadFromFile(const char* fileName)
{
    std::ifstream mapFile{fileName, std::ios_base::binary};

    int width, height;
    mapFile.read((char*)&width, sizeof(int));
    mapFile.read((char*)&height, sizeof(int));
    assert((width > 0) && (height > 0));

    TileMapTemplate result{width, height};

    // Read player spawn positions
    mapFile.read((char*)result.m_playerSpawnPositions.data(), sizeof(result.m_playerSpawnPositions));

    // Read tiles
    mapFile.read((char*)result.data(), result.size());

    return result;
}

void TileMapTemplate::saveToFile(const TileMapTemplate& tileMapTemplate, const char* fileName)
{
    std::ofstream mapFile{fileName, std::ios_base::binary};
    int width = tileMapTemplate.width();
    int height = tileMapTemplate.height();

    assert((width > 0) && (height > 0));
    mapFile.write((char*)&width, sizeof(int));
    mapFile.write((char*)&height, sizeof(int));

    // Write player spawn posititons
    mapFile.write((char*)tileMapTemplate.m_playerSpawnPositions.data(), sizeof(tileMapTemplate.m_playerSpawnPositions));

    // Write tiles
    mapFile.write((const char*)tileMapTemplate.data(), tileMapTemplate.size());
}

void TileMapTemplate::clear()
{
    std::memset(m_tiles.data(), 0, m_tiles.size() * sizeof(decltype(m_tiles)::value_type));
}

TileMapTemplate TileMapTemplate::clone() const
{
    TileMapTemplate result = *this;
    return result;
}

} // namespace game::map
