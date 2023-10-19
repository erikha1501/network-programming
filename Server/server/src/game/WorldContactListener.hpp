#pragma once

#include "entity/BombEntity.hpp"
#include "entity/Entity.hpp"
#include "entity/PlayerEntity.hpp"

#include <box2d/b2_common.h>
#include <box2d/b2_contact.h>
#include <box2d/b2_world_callbacks.h>

namespace game
{
class GameWorld;

class WorldContactListener : public b2ContactListener
{
public:
    WorldContactListener(GameWorld* gameWorld) : m_gameWorld(gameWorld)
    {
    }

public:
    void BeginContact(b2Contact* contact) override;

    void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;

private:
    GameWorld* m_gameWorld;
};

} // namespace game
