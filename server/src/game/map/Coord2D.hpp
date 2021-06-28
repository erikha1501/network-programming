#pragma once

#include <sys/types.h>
#include <functional>

namespace game::map
{
    
struct Coord2D
{
struct Hasher
{
    std::size_t operator()(const Coord2D& coord) const noexcept
    {
        std::size_t h1 = std::hash<uint32_t>{}(coord.x);
        std::size_t h2 = std::hash<uint32_t>{}(coord.y);

        return h1 ^ (h2 << 1);
    }
};

    uint x;
    uint y;

    friend bool operator==(const Coord2D& left, const Coord2D& right)
    {
        return left.x == right.x && left.y == right.y;
    }
    friend bool operator!=(const Coord2D& left, const Coord2D& right)
    {
        return !(left == right);
    }
};

} // namespace game::map
