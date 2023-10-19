#pragma once

#include "../core/constant.hpp"
#include "entity/PlayerEntity.hpp"
#include "entity/BombEntity.hpp"
#include "entity/PowerupEntity.hpp"

#include "map/TileMap.hpp"

#include "WorldContactListener.hpp"

#include <box2d/b2_world.h>

#include <array>
#include <deque>
#include <optional>
#include <unordered_map>

namespace game
{

class GameWorld
{
public:
    GameWorld();

    void addPlayer(uint id);
    void initialize();

    const map::TileMap& tileMap() const;

    PlayerEntity::InputData& playerInput(uint id);
    const PlayerEntity& playerEntity(uint id) const;

    std::vector<uint> getAlivePlayerIDs() const;

    const std::deque<BombEntity>& bombList() const;
    const std::unordered_map<map::Coord2D, PowerupEntity, map::Coord2D::Hasher>& powerupList() const;

    void applyPlayerInput();
    void physicStep(time_duration deltaTime);
    void gameLogicStep(time_duration deltaTime);
    bool checkEndCondition() const;

private:
    void tryPlaceBomb(b2Vec2 bombPosition, PlayerEntity* placer);
    void onBombDetonation(BombEntity& bombEntity);

    // Return false to stop bomb propagation in the current direction
    bool onBombHitTile(uint tileX, uint tileY);
    void onBombHitPlayer(uint positionX, uint positionY, uint playerID);
    void onBombHitBreakable(uint positionX, uint positionY);

    void onPlayerPickupPowerup(PlayerEntity* playerEntity, PowerupEntity* powerupEntity);

    // Must be careful not to add the same body twice
    void registerBodyDestruction(b2Body* body);

    void executeBodyDestructionRoutine();

private:
    std::array<PlayerEntity, g_maxPlayerCount> m_playerList;
    std::deque<BombEntity> m_bombList;
    std::unordered_map<map::Coord2D, PowerupEntity, map::Coord2D::Hasher> m_powerupList;

    map::TileMap m_tileMap;

    b2World m_world;

    friend class WorldContactListener;
    WorldContactListener m_contactListener;

    std::deque<b2Body*> m_bodiesToBeDestroyed;
};

} // namespace game
