using System;
using System.Collections.Generic;

using UnityEditor;
using UnityEngine;

using BoomOnline2.GamePlay;
using System.Runtime.InteropServices;

namespace BoomOnline2.Editor
{

    [CustomEditor(typeof(GameWorldController))]
    public class GameWorldControllerEditor : UnityEditor.Editor
    {
        GameWorldController inspectedController;

        bool showMapGenerate = true;
        string fileName = "Map/default.bomap";


        private void OnEnable()
        {
            inspectedController = (GameWorldController)target;
        }

        public override void OnInspectorGUI()
        {
            DrawDefaultInspector();

            GUILayout.Space(10);

            showMapGenerate = EditorGUILayout.Foldout(showMapGenerate, "Map generation");

            if (!showMapGenerate)
            {
                return;
            }

            fileName = GUILayout.TextField(fileName);

            if (GUILayout.Button("Generate map"))
            {
                Span<byte> mapFileSpan = LoadMapFile(fileName);

                int width = MemoryMarshal.Read<int>(mapFileSpan.Slice(0, sizeof(int)));
                mapFileSpan = mapFileSpan.Slice(sizeof(int));

                int height = MemoryMarshal.Read<int>(mapFileSpan.Slice(0, sizeof(int)));
                mapFileSpan = mapFileSpan.Slice(sizeof(int));

                Debug.Log($"Reading map file \"{fileName}\": (width, height) = ({width}, {height})");

                const int playerSpawnPositionsSize = sizeof(uint) * 2 * GameConstant.MaxPlayerCount;
                Span<byte> playerSpawnPositionsSpan = mapFileSpan.Slice(0, playerSpawnPositionsSize);

                Vector2Int[] playerSpawnPositions = new Vector2Int[GameConstant.MaxPlayerCount];
                for (int i = 0; i < playerSpawnPositions.Length; i++)
                {
                    int x = MemoryMarshal.Read<int>(playerSpawnPositionsSpan.Slice(0, sizeof(int)));
                    playerSpawnPositionsSpan = playerSpawnPositionsSpan.Slice(sizeof(int));

                    int y = MemoryMarshal.Read<int>(playerSpawnPositionsSpan.Slice(0, sizeof(int)));
                    playerSpawnPositionsSpan = playerSpawnPositionsSpan.Slice(sizeof(int));

                    playerSpawnPositions[i] = new Vector2Int(x, y);
                }

                Span<byte> mapDataSpan = mapFileSpan.Slice(playerSpawnPositionsSize);
                Debug.Assert(mapDataSpan.Length == width * height);

                inspectedController.SetupGameWorldEditMode(width, height, mapDataSpan, playerSpawnPositions);
            }

            if (GUILayout.Button("Clear map"))
            {
                inspectedController.ClearGameWorldEditMode();
            }
        }

        private byte[] LoadMapFile(string fileName)
        {
            TextAsset mapAsset = Resources.Load<TextAsset>(fileName);
            return mapAsset.bytes;
        }
    }
}