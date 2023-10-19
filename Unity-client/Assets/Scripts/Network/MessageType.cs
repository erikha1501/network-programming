namespace Pnet
{
    enum MessageType : byte
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
}

namespace Pgame
{
    enum MessageType : byte
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
}
