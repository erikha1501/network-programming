#pragma once

namespace pnet
{

enum class MessageType : uint8_t
{
    ConnectionPing = 0,
    ConnectionPong = 1,

    ServerConnectionPing = 2,
    ServerConnectionPong = 3,

    QueryRoomInfoRequest = 4,
    QueryRoomInfoResponse = 5,

    CreateRoomRequest = 6,
    CreateRoomResponse = 7,

    JoinRoomRequest = 8,
    JoinRoomResponse = 9,

    LeaveRoomRequest = 10,
    LeaveRoomResponse = 11,

    QuerySelfRoomInfoRequest = 12,
    RoomInfoChanged = 13,

    StartGameRequest = 14,
    StartGameResponse = 15,
    GameCreated = 16
};

} // namespace pnet
