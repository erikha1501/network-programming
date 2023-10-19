using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Pgame;
using static Pgame.GameState.Types;
using Unity.VisualScripting;

namespace BoomOnline2.GamePlay
{
    public class GameWorldController : MonoBehaviour
    {
        private Vector2 worldSize;

        [SerializeField]
        private Vector2 worldBoundMarginLeftRight;
        [SerializeField]
        private Vector2 worldBoundMarginTopBottom;

        [SerializeField]
        private Transform mapRoot;
        [SerializeField]
        private Transform playerTransformGroup;

        #region Prefab
        [SerializeField]
        private GameObject[] playerPrefabs;

        [SerializeField]
        private GameObject[] groundTilePrefabs;
        [SerializeField]
        private GameObject wallTilePrefab;
        [SerializeField]
        private GameObject[] breakableTilePrefabs;

        [SerializeField]
        private GameObject bombPrefab;

        [SerializeField]
        private GameObject powerupBombRangePrefab;
        [SerializeField]
        private GameObject powerupBombQuantityPrefab;
        [SerializeField]
        private GameObject powerupSpeedupPrefab;
        #endregion

        private Camera gameCamera;

        #region Game data
        TileMap tileMap;

        Dictionary<uint, EntityController> entityList;
        List<KeyValuePair<uint, EntityController>> invalidEntityList;
        #endregion

        private Vector2Int[] playerSpawnPositions;
        private PlayerController[] playerList;

        private uint playerSelfID;

        private void Awake()
        {
            gameCamera = Camera.main;

            playerSelfID = PlayerSelfInfo.Instance.PlayerID;

            entityList = new Dictionary<uint, EntityController>();
            invalidEntityList = new List<KeyValuePair<uint, EntityController>>();
        }

        public PlayerController.PlayerState SelfState()
        {
            return playerList[playerSelfID].State;
        }

        public void SetupGameWorld(int width, int height, ReadOnlySpan<byte> mapData, Vector2Int[] playerSpawnPositions)
        {
            ClearGameWorld();

            worldSize = new Vector2(width, height);
            float marginX = worldBoundMarginLeftRight.x + worldBoundMarginLeftRight.y;
            float marginY = worldBoundMarginTopBottom.x + worldBoundMarginTopBottom.y;

            // Setup camera
            Vector2 cameraHalfSize = (worldSize + new Vector2(marginX, marginY)) * 0.5f;
            gameCamera.transform.position = new Vector3(cameraHalfSize.x, cameraHalfSize.y, -10.0f) - new Vector3(worldBoundMarginLeftRight.x, worldBoundMarginTopBottom.y);
            gameCamera.orthographicSize = cameraHalfSize.y;
            gameCamera.aspect = cameraHalfSize.x / cameraHalfSize.y;

            // Setup screen resolution
            float screenRatio = cameraHalfSize.x / cameraHalfSize.y;
            int screenHeight = 720;
            int screenWidth = (int)((float)screenHeight * screenRatio);
            Screen.SetResolution(screenWidth, screenHeight, false);


            // Create tile map
            tileMap = new TileMap(width, height);


            // Setup tiles
            for (int y = 0; y < tileMap.Height; y++)
            {
                for (int x = 0; x < tileMap.Width; x++)
                {
                    byte tileData = mapData[x + y * tileMap.Width];
                    TileType tileType = (TileType)(tileData & Tile.MaskTileType);
                    int tileDetails = (tileData & ~Tile.MaskTileType) >> Tile.TileDetailsMaskShift;

                    GameObject.Instantiate(groundTilePrefabs[(x + y) % 2], new Vector3(x, y) + new Vector3(0.5f, 0.5f), Quaternion.identity, mapRoot);

                    GameObject tileGameObject = null;
                    switch (tileType)
                    {
                        case TileType.Empty:
                            break;
                        case TileType.Wall:
                            tileGameObject = GameObject.Instantiate(wallTilePrefab, new Vector3(x, y) + new Vector3(0.5f, 0.5f), Quaternion.identity, mapRoot);
                            break;
                        case TileType.Breakable:
                            tileGameObject = GameObject.Instantiate(breakableTilePrefabs[tileDetails], new Vector3(x, y) + new Vector3(0.5f, 0.5f), Quaternion.identity, mapRoot);
                            break;
                        default:
                            break;
                    }

                    tileMap[x, y] = new Tile(tileGameObject, tileData);
                }
            }


            // Save player spawn positions
            this.playerSpawnPositions = playerSpawnPositions;
            playerList = new PlayerController[GameConstant.MaxPlayerCount];
        }

        public void RegisterPlayers(IEnumerable<uint> playerIDs)
        {
            foreach (uint playerID in playerIDs)
            {
                GameObject playerObject = GameObject.Instantiate(playerPrefabs[playerID], (Vector3Int)playerSpawnPositions[playerID] + new Vector3(0.5f, 0.5f), Quaternion.identity, playerTransformGroup);
                playerList[playerID] = playerObject.GetComponent<PlayerController>();
                playerList[playerID].SetName(PlayerSelfInfo.Instance.Room[(int)playerID].Name);
            }
        }

        public void UpdateGameState(GameState gameState)
        {
            // Update player states
            foreach (var playerState in gameState.PlayerStates)
            {
                PlayerController playerController = playerList[playerState.PlayerId];
                if (playerController.State == PlayerController.PlayerState.Dead)
                {
                    continue;
                }

                if (playerController.State == PlayerController.PlayerState.Alive && playerState.State == PlayerState.Types.State.Dead)
                {
                    playerController.Die();
                    continue;
                }

                // Update positions and speeds
                playerController.transform.position = new Vector3(playerState.PositionX, playerState.PositionY);
                playerController.SetVelocity(new Vector2(playerState.DirectionX, playerState.DirectionY));
            }

            // Update bomb states
            foreach (var bombState in gameState.BombStates)
            {
                if (entityList.TryGetValue(bombState.Id, out EntityController entityController))
                {
                    // Keep this entity alive
                    entityController.Valid = true;
                }
                else
                {
                    // New bomb placed
                    GameObject newBombObject = GameObject.Instantiate(bombPrefab, new Vector3(bombState.PositionX, bombState.PositionY) + new Vector3(0.5f, 0.5f), Quaternion.identity);
                    BombController newBombController = newBombObject.GetComponent<BombController>();
                    newBombController.Valid = true;
                    newBombController.TilePosition = new Vector2Int((int)bombState.PositionX, (int)bombState.PositionY);
                    newBombController.DetonationRange = bombState.Range;
                    newBombController.TileMap = tileMap;

                    entityList.Add(bombState.Id, newBombController);
                }
            }

            // Update powerup states
            foreach (var powerupState in gameState.PowerupStates)
            {
                if (entityList.TryGetValue(powerupState.Id, out EntityController entityController))
                {
                    // Keep this entity alive
                    entityController.Valid = true;
                }
                else
                {
                    // New powerup spawned
                    GameObject powerupPrefab = powerupState.PowerupType switch
                    {
                        PowerupState.Types.Type.BombRange => powerupBombRangePrefab,
                        PowerupState.Types.Type.BombQuantity => powerupBombQuantityPrefab,
                        PowerupState.Types.Type.Speedup => powerupSpeedupPrefab,
                        _ => null
                    };

                    if (powerupPrefab != null)
                    {
                        GameObject powerupObject = GameObject.Instantiate(powerupPrefab, new Vector3(powerupState.PositionX, powerupState.PositionY) + new Vector3(0.5f, 0.5f), Quaternion.identity);
                        PowerupController newPowerupController = powerupObject.GetComponent<PowerupController>();
                        newPowerupController.Valid = true;
                        entityList.Add(powerupState.Id, newPowerupController);
                    }
                    else
                    {
                        Debug.Log("Unknown powerup");
                    }
                }
            }

            // Collect invalid entities
            foreach (var entityEntry in entityList)
            {
                if (!entityEntry.Value.Valid)
                {
                    invalidEntityList.Add(entityEntry);
                }

                // Invalidate for the next game state update
                entityEntry.Value.Valid = false;
            }

            // Destroy invalid entities
            foreach (var entityEntry in invalidEntityList)
            {
                entityList.Remove(entityEntry.Key);
                entityEntry.Value.DestroySelf();
            }

            invalidEntityList.Clear();


            // Update map
            for (int y = 0; y < tileMap.Height; y++)
            {
                for (int x = 0; x < tileMap.Width; x++)
                {
                    ref Tile tile = ref tileMap[x, y];
                    byte oldTileData = tile.TileData;
                    byte newTileData = gameState.MapData[x + y * tileMap.Width];

                    if ((oldTileData ^ newTileData) == 0)
                    {
                        continue;
                    }

                    TileType oldTileType = tile.Type();
                    TileType newTileType = (TileType)(newTileData & Tile.MaskTileType);

                    if (oldTileType == TileType.Breakable && newTileType == TileType.Empty)
                    {
                        // Handle tile breaking
                        Destroy(tile.TileGameObject);
                        tile.TileData = newTileData;
                        tile.TileGameObject = null;
                    }
                }
            }
        }

        public void ClearGameWorld()
        {
            int tileCount = mapRoot.childCount;
            for (int i = tileCount - 1; i >= 0; i--)
            {
                Destroy(mapRoot.GetChild(i).gameObject);
            }

            int playerCount = playerTransformGroup.childCount;
            for (int i = playerCount - 1; i >= 0; i--)
            {
                Destroy(playerTransformGroup.GetChild(i).gameObject);
            }
        }

#if UNITY_EDITOR
        public void SetupGameWorldEditMode(int width, int height, ReadOnlySpan<byte> mapData, Vector2Int[] playerSpawnPositions)
        {
            ClearGameWorldEditMode();

            gameCamera = Camera.main;

            SetupGameWorld(width, height, mapData, playerSpawnPositions);
        }

        public void ClearGameWorldEditMode()
        {
            int tileCount = mapRoot.childCount;
            for (int i = tileCount - 1; i >= 0; i--)
            {
                DestroyImmediate(mapRoot.GetChild(i).gameObject);
            }

            int playerCount = playerTransformGroup.childCount;
            for (int i = playerCount - 1; i >= 0; i--)
            {
                DestroyImmediate(playerTransformGroup.GetChild(i).gameObject);
            }
        }
#endif
    }
}
