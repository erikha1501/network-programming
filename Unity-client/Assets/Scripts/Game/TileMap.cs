using UnityEngine;

namespace BoomOnline2.GamePlay
{
    public class TileMap
    {
        public readonly Tile[] Tiles;
        public readonly int Width;
        public readonly int Height;

        public TileMap(int width, int height)
        {
            Tiles = new Tile[width * height];
            Width = width;
            Height = height;
        }

        public ref Tile this[int x, int y]
        {
            get => ref Tiles[x + y * Width];
        }
    }
}