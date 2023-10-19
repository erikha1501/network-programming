#pragma once

#include <cstdint>
#include <sys/types.h>

namespace game
{

class Entity
{
public:
    enum class EntityType
    {
        Unknown = 0,
        Player = 1,
        Bomb = 2,
        Powerup = 3
    };

    static constexpr uint16_t playerCategoryBits = 0x2;
    static constexpr uint16_t playerMaskBits = 0xffff & (~playerCategoryBits);

protected:
    Entity(EntityType type) : m_isActive(true), m_type(type)
    {
        m_id = nextID();
    }

public:
    EntityType type() const
    {
        return m_type;
    }

    uint id() const
    {
        return m_id;
    }

    bool isActive() const
    {
        return m_isActive;
    }
    void setActive(bool active)
    {
        m_isActive = active;
    }

protected:
    bool m_isActive;

private:
    static uint nextID();

private:
    EntityType m_type;

    uint m_id;
};

} // namespace game
