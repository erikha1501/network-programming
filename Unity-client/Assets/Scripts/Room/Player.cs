using System.Collections.Generic;
using System.Net;

namespace BoomOnline2
{
    public class Player
    {
        public uint ID { get; private set; }
        public string Name { get; private set; }

        public Player(uint id, string name)
        {
            ID = id;
            Name = name;
        }
    } 
}
