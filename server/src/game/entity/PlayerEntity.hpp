#pragma once

#include "Entity.hpp"
#include "PlayerStats.hpp"

#include <box2d/b2_body.h>
#include <box2d/b2_math.h>

namespace game
{

class PlayerEntity : public Entity
{
public:
    enum class State
    {
        Invalid = 0,
        Dead = 1,
        Alive = 2
    };
    struct InputData
    {
        b2Vec2 direction;
        bool bombPlaced;
    };

public:
    PlayerEntity() : Entity(EntityType::Player), m_state(State::Invalid), m_inputData{}, m_stats{}, m_body{}
    {
    }

    PlayerEntity(b2Body* body) : Entity(EntityType::Player), m_state(State::Alive), m_inputData{}, m_stats{}, m_body(body)
    {
    }

    State state() const
    {
        return m_state;
    }
    void setState(State state)
    {
        m_state = state;
    }

    InputData& inputData()
    {
        return m_inputData;
    }

    PlayerStats& stats()
    {
        return m_stats;
    }

    b2Body* body() const
    {
        return m_body;
    }

private:
    State m_state;

    InputData m_inputData;

    PlayerStats m_stats;

    b2Body* m_body;
};

} // namespace game
