using System.Net;

namespace BoomOnline2
{
    public class GameConfiguration
    {
        private static readonly GameConfiguration instance = new GameConfiguration();
        public static GameConfiguration Instance => instance;


        public ProtobufSocket RoomProtobufSocket { get; }

        public IPAddress ServerAddress { get; private set; }
        public uint RoomServerPort { get; private set; }

        private GameConfiguration()
        {
            ServerAddress = IPAddress.Loopback;
            RoomServerPort = 6969;

            RoomProtobufSocket = new ProtobufSocket(ServerAddress, (int)RoomServerPort, false);
        }

        public void UpdateServerEndpoint(IPAddress address)
        {
            ServerAddress = address;
            RoomProtobufSocket.Connect(address, (int)RoomServerPort);
        }

        public void UpdateServerEndpoint(IPAddress address, uint port)
        {
            RoomServerPort = port;
            RoomProtobufSocket.Connect(address, (int)port);
        }
    }
}