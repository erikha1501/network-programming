using UnityEngine;

using BoomOnline2.GamePlay;

namespace BoomOnline2
{
    public class BombController : EntityController
    {
        [SerializeField]
        private GameObject explosionAnimationPrefab;

        [SerializeField]
        private GameObject firePrefab;

        [SerializeField]
        private float detonationAnimationDuration = 1.0f;

        private static readonly Vector2Int[] direction =
        {
            Vector2Int.left,
            Vector2Int.up,
            Vector2Int.right,
            Vector2Int.down
        };

        private static readonly Vector3 tileOffset = new Vector3(0.5f, 0.5f);

        public Vector2Int TilePosition { get; set; }

        public TileMap TileMap { get; set; }

        public uint DetonationRange { get; set; }

        public override void DestroySelf()
        {
            OnDetonation();
            Destroy(this.gameObject);
        }

        private void OnDetonation()
        {
            GameObject explosionAnimationObject = GameObject.Instantiate(explosionAnimationPrefab, new Vector3(TilePosition.x, TilePosition.y) + tileOffset, Quaternion.identity);
            Destroy(explosionAnimationObject, detonationAnimationDuration);


            for (int dirIndex = 0; dirIndex < direction.Length; dirIndex++)
            {
                for (int r = 1; r <= DetonationRange; r++)
                {
                    Vector2Int detonationPos = TilePosition + direction[dirIndex] * r;

                    // Bounds check
                    if (detonationPos.x < 0 || detonationPos.x >= TileMap.Width ||
                        detonationPos.y < 0 || detonationPos.y >= TileMap.Height)
                    {
                        break;
                    }

                    // Stop propagation at wall or breakable
                    TileType tileType = TileMap[detonationPos.x, detonationPos.y].Type();
                    if (tileType == TileType.Breakable || tileType == TileType.Wall)
                    {
                        break;
                    }

                    GameObject fireObject = GameObject.Instantiate(firePrefab, new Vector3(detonationPos.x, detonationPos.y) + tileOffset, Quaternion.identity);
                    Destroy(fireObject, detonationAnimationDuration);
                }
            }
        }
    }
}