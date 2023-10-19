#include "WorldContactListener.hpp"

#include "GameWorld.hpp"

namespace game
{

void WorldContactListener::BeginContact(b2Contact* contact)
{
    if (!contact->IsTouching())
    {
        return;
    }

    b2Body* bodyA = contact->GetFixtureA()->GetBody();
    b2Body* bodyB = contact->GetFixtureB()->GetBody();

    Entity* const entityA = (Entity*)(bodyA->GetUserData().pointer);
    Entity* const entityB = (Entity*)(bodyB->GetUserData().pointer);

    // Only deal with entity collision
    if (entityA == nullptr || entityB == nullptr)
    {
        return;
    }

    if (!entityA->isActive() || !entityB->isActive())
    {
        return;
    }
    
    PlayerEntity* playerEntity = nullptr;
    PowerupEntity* powerupEntity = nullptr;

    if (entityA->type() == Entity::EntityType::Player)
    {
        playerEntity = (PlayerEntity*)entityA;
    }
    else if (entityA->type() == Entity::EntityType::Powerup)
    {
        powerupEntity = (PowerupEntity*)entityA;
    }

    if (entityB->type() == Entity::EntityType::Player)
    {
        playerEntity = (PlayerEntity*)entityB;
    }
    else if (entityB->type() == Entity::EntityType::Powerup)
    {
        powerupEntity = (PowerupEntity*)entityB;
    }

    if (playerEntity == nullptr || powerupEntity == nullptr)
    {
        return;
    }

    m_gameWorld->onPlayerPickupPowerup(playerEntity, powerupEntity);
}

void WorldContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
    b2Body* bodyA = contact->GetFixtureA()->GetBody();
    b2Body* bodyB = contact->GetFixtureB()->GetBody();

    const Entity* entityA = (Entity*)(bodyA->GetUserData().pointer);
    const Entity* entityB = (Entity*)(bodyB->GetUserData().pointer);

    // Only deal with entity collision
    if (entityA == nullptr || entityB == nullptr)
    {
        return;
    }

    b2Body* playerBody = nullptr;
    b2Body* bombBody = nullptr;

    if (entityA->type() == Entity::EntityType::Player)
    {
        playerBody = bodyA;
    }
    else if (entityA->type() == Entity::EntityType::Bomb)
    {
        bombBody = bodyA;
    }

    if (entityB->type() == Entity::EntityType::Player)
    {
        playerBody = bodyB;
    }
    else if (entityB->type() == Entity::EntityType::Bomb)
    {
        bombBody = bodyB;
    }

    if (playerBody == nullptr || bombBody == nullptr)
    {
        return;
    }

    // Test distance
    b2Vec2 distance = playerBody->GetPosition() - bombBody->GetPosition();
    // TODO: Use global constant
    constexpr float distanceThreshold = (0.2f + 0.5f);
    if (distance.LengthSquared() <= distanceThreshold * distanceThreshold)
    {
        contact->SetEnabled(false);
    }
}

} // namespace game
