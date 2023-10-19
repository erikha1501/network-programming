using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace BoomOnline2.GamePlay
{
    public enum TileType : byte
    {
        Empty = 0,
        Wall = 1,
        Breakable = 2
    };

    public struct Tile
    {
        public const byte MaskTileType = 0b000000_11;
        public const byte TileDetailsMaskShift = 2;

        public GameObject TileGameObject { get; set; }
        public byte TileData { get; set; }

        public Tile(GameObject tileGameObject, byte tileData)
        {
            TileGameObject = tileGameObject;
            TileData = tileData;
        }

        public TileType Type()
        {
            return (TileType)(TileData & MaskTileType);
        }
    }
}