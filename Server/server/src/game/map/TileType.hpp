#pragma once

#include <cstdint>

namespace game::map
{

enum class TileType : uint8_t
{
    Empty = 0,
    Wall = 1,
    Breakable = 2
};

} // namespace game::map
