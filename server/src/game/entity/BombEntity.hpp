#pragma once

#include "../../core/constant.hpp"

#include "Entity.hpp"
#include "PlayerEntity.hpp"

#include <box2d/b2_body.h>
#include <box2d/b2_math.h>

namespace game
{

class BombEntity : public Entity
{
public:
    BombEntity(uint positionX, uint positionY, uint range, b2Body* body, PlayerEntity* placer)
        : Entity(EntityType::Bomb), m_positionX(positionX), m_positionY(positionY), m_range(range),
          m_detonationTimer(s_bombDetonationTime), m_placer(placer), m_body(body)
    {
    }

public:
    uint positionX() const
    {
        return m_positionX;
    }
    uint positionY() const
    {
        return m_positionY;
    }

    uint range() const
    {
        return m_range;
    }

    time_duration detonationTimer() const
    {
        return m_detonationTimer;
    }
    void tickDetonationTimer(time_duration deltaTime)
    {
        m_detonationTimer -= deltaTime;
    }
    bool hasDetonated() const
    {
        return m_detonationTimer <= time_duration(0.0f);
    }

    PlayerEntity* placer() const
    {
        return m_placer;
    }
    b2Body* body() const
    {
        return m_body;
    }

private:
    constexpr static float s_bombDetonationTime = 3.0f;

private:
    uint m_positionX;
    uint m_positionY;

    uint m_range;

    time_duration m_detonationTimer;

    PlayerEntity* m_placer;

    b2Body* m_body;
};

} // namespace game
