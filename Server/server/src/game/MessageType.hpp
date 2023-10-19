#pragma once

namespace pgame
{

enum class MessageType : uint8_t
{
    JoinGameRequest = 0,
    JoinGameResponse = 1,
    MapInfoReceived = 2,
    GameStatusChanged = 3,
    ConnectionPing = 4,
    ConnectionPong = 5,
    PlayerInput = 6,
    GameState = 7
};

} // namespace game
