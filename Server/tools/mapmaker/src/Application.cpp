#include "../../../server/src/game/map/TileMapTemplate.hpp"

#include <stdio.h>
#include <cstring>

int main()
{
    constexpr auto __ = (uint8_t)game::map::TileType::Empty;
    constexpr auto WW = (uint8_t)game::map::TileType::Wall;
    constexpr auto BB = (uint8_t)game::map::TileType::Breakable;

    constexpr uint8_t B0 = 0 << game::map::TileMapTemplate::s_tileDetailsMaskShift;
    constexpr uint8_t B1 = 1 << game::map::TileMapTemplate::s_tileDetailsMaskShift;
    constexpr uint8_t B2 = 2 << game::map::TileMapTemplate::s_tileDetailsMaskShift;
    constexpr uint8_t B3 = 3 << game::map::TileMapTemplate::s_tileDetailsMaskShift;

    constexpr int width = 16;
    constexpr int height = 13;

    game::map::TileMapTemplate mapTemplate{width, height};

    uint8_t tiles[width * height] =
    {
        WW, __, BB, WW, BB, WW, BB, BB, BB, BB, BB, WW, BB, WW, __, WW,
        __, __, __, __, BB, BB, BB, WW, WW, WW, BB, BB, BB, __, __, __,
        WW, __, WW, WW, BB, WW, BB, WW, WW, WW, BB, WW, BB, WW, __, WW,
        BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB,
        WW, BB, BB, BB, WW, BB, WW, BB, WW, BB, WW, BB, WW, BB, BB, WW,
        BB, BB, BB, WW, BB, BB, BB, __, __, __, __, WW, __, WW, BB, BB,
        WW, __, BB, BB, WW, BB, WW, WW, WW, WW, WW, BB, WW, BB, __, WW,
        __, __, WW, WW, BB, BB, BB, BB, BB, BB, BB, BB, BB, WW, __, __,
        BB, WW, WW, BB, WW, BB, WW, BB, WW, BB, WW, BB, WW, BB, WW, BB,
        BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB,
        BB, BB, WW, BB, WW, BB, WW, BB, WW, BB, WW, BB, WW, BB, WW, BB,
        WW, __, __, BB, __, BB, BB, BB, BB, BB, BB, BB, __, BB, __, __,
        WW, __, WW, BB, WW, BB, WW, BB, WW, BB, WW, BB, WW, BB, WW, __,
    };

    uint8_t tiles_detail[width * height] =
    {
        __, __, __, __, __, __, B3, B3, B3, B2, B3, __, BB, __, __, __,
        __, __, __, __, BB, BB, B3, __, __, __, B3, BB, BB, __, __, __,
        __, __, __, __, BB, __, B2, __, __, __, B3, __, BB, __, __, __,
        BB, BB, BB, BB, BB, B1, B3, B3, B2, B3, B3, BB, BB, BB, BB, BB,
        __, BB, BB, BB, __, B1, __, B1, __, B1, __, B1, __, BB, BB, __,
        BB, BB, BB, __, BB, B1, B1, __, __, __, __, __, __, __, BB, BB,
        __, __, BB, BB, __, B1, __, __, __, __, __, B1, __, BB, __, __,
        __, __, __, __, BB, B1, B1, B1, B1, B1, B1, B1, BB, __, __, __,
        BB, __, __, BB, __, BB, __, BB, __, BB, __, BB, __, BB, __, BB,
        B1, B1, B1, B1, B1, B1, B1, B1, B1, B1, B1, B1, B1, B1, B1, B1,
        BB, BB, __, BB, __, BB, __, BB, __, BB, __, BB, __, BB, __, BB,
        __, __, __, BB, __, BB, BB, BB, BB, BB, BB, BB, __, BB, __, __,
        __, __, __, BB, __, BB, __, BB, __, BB, __, BB, __, BB, __, __,   
    };

    // Copy tiles to map template (with y-axis flipped)
    uint8_t* mapTemplateTiles = (uint8_t*)mapTemplate.data();
    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            mapTemplateTiles[x + (height - 1 - y) * width] = 
            (tiles[x + y * width] & game::map::TileMapTemplate::s_tileTypeMask) |
            tiles_detail[x + y * width];
        }
    }


    mapTemplate.playerSpawnPositions()[0] = {1, 11};
    mapTemplate.playerSpawnPositions()[1] = {14, 11};
    mapTemplate.playerSpawnPositions()[2] = {1, 5};
    mapTemplate.playerSpawnPositions()[3] = {14, 5};

    // Print map template
    for (int y = 0; y < mapTemplate.height(); y++)
    {
        for (int x = 0; x < mapTemplate.width(); x++)
        {
            printf("%d ", (uint8_t)mapTemplate.atPosition(x, mapTemplate.height() - 1 - y));
        }

        printf("\n");
    }

    printf("Spawn positions:\n");
    for (int i = 0; i < mapTemplate.playerSpawnPositions().size(); i++)
    {
        auto [x, y] = mapTemplate.playerSpawnPositions()[i];
        printf("P%d: (%d, %d)\n", i, x, y);
    }
    printf("\n");

    game::map::TileMapTemplate::saveToFile(mapTemplate);
}
