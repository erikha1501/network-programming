#pragma once

#include "../../core/constant.hpp"

#include "Entity.hpp"
#include "PlayerEntity.hpp"

#include <box2d/b2_body.h>
#include <box2d/b2_math.h>

namespace game
{

class PowerupEntity : public Entity
{
public:
    enum class Type : uint8_t
    {
        BombQuantity,
        BombRange,
        Speedup
    };

public:
    PowerupEntity(uint positionX, uint positionY, Type powerupType)
        : Entity(EntityType::Powerup), m_positionX(positionX), m_positionY(positionY), m_powerupType(powerupType),
          m_body(nullptr)
    {
        m_isActive = false;
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

    Type powerupType() const
    {
        return m_powerupType;
    }

    void assignBody(b2Body* powerupBody)
    {
        m_body = powerupBody;
    }

    b2Body* body() const
    {
        return m_body;
    }

private:
    uint m_positionX;
    uint m_positionY;

    Type m_powerupType;

    b2Body* m_body;
};

} // namespace game
