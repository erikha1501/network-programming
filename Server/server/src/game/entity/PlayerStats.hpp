#pragma once

#include "../../core/CommonTypes.hpp"

#include <sys/types.h>
#include <assert.h>

namespace game
{

class PlayerStats
{
public:
    PlayerStats()
    {
        m_carryingBombCapacity = s_initialBombCount;
        m_carryingBombCount = s_initialBombCount;
        m_bombRange = s_intialBombRange;
        m_speed = s_initialSpeed;
    }

public:
    uint carryingBombCount() const
    {
        return m_carryingBombCount;
    }

    uint bombRange() const
    {
        return m_bombRange;
    }

    float speed() const
    {
        return m_speed;
    }

    void incrementCarryingBombCount()
    {
        assert(m_carryingBombCount < m_carryingBombCapacity);
        m_carryingBombCount++;
    }

    void decrementCarryingBombCount()
    {
        assert(m_carryingBombCount > 0);
        m_carryingBombCount--;
    }

    void upgradeBombCapacity()
    {
        uint newBombCapacity = m_carryingBombCapacity + s_bombCountPowerup;

        if (newBombCapacity <= s_maxBombCount)
        {
            m_carryingBombCapacity = newBombCapacity;
            m_carryingBombCount += s_bombCountPowerup;
        }
    }

    void upgradeBombRange()
    {
        uint newBombRange = m_bombRange + s_bombRangePowerup;

        if (newBombRange <= s_maxBombRange)
        {
            m_bombRange = newBombRange;
        }
    }

    void upgradeSpeed()
    {
        float newSpeed = m_speed + s_speedPowerup;

        if (newSpeed <= s_maxSpeed)
        {
            m_speed = newSpeed;
        }
    }

private:
    static constexpr time_duration s_bombDetonationTime = time_duration(5.0f);

    static constexpr uint s_initialBombCount = 1;
    static constexpr uint s_maxBombCount = 5;
    static constexpr uint s_bombCountPowerup = 1;

    static constexpr uint s_intialBombRange = 1;
    static constexpr uint s_maxBombRange = 5;
    static constexpr uint s_bombRangePowerup = 1;

    static constexpr float s_initialSpeed = 3.0f;
    static constexpr float s_maxSpeed = 5.0f;
    static constexpr float s_speedPowerup = 0.5f;

private:
    uint m_carryingBombCapacity;
    uint m_carryingBombCount;
    uint m_bombRange;
    float m_speed;
};

} // namespace game
