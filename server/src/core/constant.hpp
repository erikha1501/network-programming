#pragma once

#include "CommonTypes.hpp"

#include <sys/types.h>

constexpr uint g_maxPlayerCount = 4;
constexpr uint g_minPlayerCount = 2;

namespace game
{

// Constant
constexpr time_duration g_playerConnectionWaitTimeout = time_duration{10.0f};
constexpr time_duration g_gameStartingCountdownInterval = time_duration{3.0f};

constexpr time_duration g_playerHeartbeatTimeout = time_duration{5.0f};
constexpr time_duration g_gameMaxDuration = time_duration{300.0f};

constexpr uint g_gameEndConditionPlayerAliveCount = 1;

} // namespace game

namespace room
{

constexpr time_duration g_playerHeartbeatTimeout = time_duration{10.0f};

} // namespace room

