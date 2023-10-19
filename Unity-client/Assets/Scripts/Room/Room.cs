using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using BoomOnline2.GamePlay;
using Pnet;

namespace BoomOnline2
{
    public class Room
    {
        public uint ID { get; private set; }
        public uint OwnerID { get; private set; }

        public bool IsFull { get; private set; }

        public IReadOnlyList<Player> Players => playerList;

        private readonly List<Player> playerList;
        private readonly Player[] indexToPlayer;

        public Player GetOwner()
        {
            foreach (var player in playerList)
            {
                if (player.ID == OwnerID)
                {
                    return player;
                }
            }

            return null;
        }

        public Player this[int playerID] => indexToPlayer[playerID];

        public Room()
        {
            playerList = new List<Player>();
            indexToPlayer = new Player[GameConstant.MaxPlayerCount];
        }

        public Room(RoomInfo roomInfo) : this()
        {
            ID = roomInfo.Id;
            OwnerID = roomInfo.OwnerId;
            IsFull = roomInfo.IsFull;

            foreach (var client in roomInfo.Clients)
            {
                Player newPlayer = new Player(client.Id, client.Name);
                playerList.Add(newPlayer);
                indexToPlayer[client.Id] = newPlayer;
            }
        }
    }
}