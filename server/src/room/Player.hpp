#pragma once

#include "../core/IPEndpoint.hpp"

#include <string>

namespace room
{

class Player
{
public:
    Player() = default;

    Player(std::string name, IPEndpoint endpoint) : m_name(std::move(name)), m_endpoint(std::move(endpoint))
    {
    }

    const std::string& name() const
    {
        return m_name;
    }
    const IPEndpoint& endpoint() const
    {
        return m_endpoint;
    }

private:
    std::string m_name;
    IPEndpoint m_endpoint;
};

} // namespace room