namespace BoomOnline2
{
    public class PlayerSelfInfo
    {
        public enum Status
        {
            InLogin,
            InLobby,
            InRoom,
            InGame
        }

        private static readonly PlayerSelfInfo instance = new PlayerSelfInfo();

        public static PlayerSelfInfo Instance => instance;

        public string Name { get; set; } 
        public uint PlayerID { get; set; }
        public uint RoomID { get; set; }

        public Room Room { get; set; }

        public uint GameServerPort { get; set; }

        public Status PlayerStatus { get; set; }

        private PlayerSelfInfo()
        {
        }
    }
}