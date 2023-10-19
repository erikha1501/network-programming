using System.Collections;
using System.Collections.Generic;
using System.Buffers;

using UnityEngine;

using Pgame;

using BoomOnline2.GamePlay;
using BoomOnline2.UI;
using Google.Protobuf;

namespace BoomOnline2
{
    public class GameManager : MonoBehaviour
    {
        enum ConnectionState
        {
            Disconnected,
            Connecting,
            Connected,
            Playing
        }

        [SerializeField]
        private GameWorldController gameWorldController;
        [SerializeField]
        private GameOverlayUIController gameOverlayUIController;
        [SerializeField]
        private GameEndUIController gameEndUIController;


        private ProtobufSocket protobufSocket;
        uint clientMessageSequenceNum;
        uint serverMessageSequenceNum;
        private PlayerInput playerInput;
        private PlayerIdentity playerIdentity;
        private ConnectionPong connectionPong;

        private InputManager inputManager;

        #region Global data
        PlayerSelfInfo playerSelfInfo;
        GameConfiguration gameConfiguration;
        #endregion

        private ConnectionState state;
        private bool canSendInput;

        private bool errorOccured;

        private List<uint> winnerIDs;

        private JsonFormatter jsonFormatter;

        void Awake()
        {
            playerSelfInfo = PlayerSelfInfo.Instance;
            gameConfiguration = GameConfiguration.Instance;

            inputManager = GetComponentInChildren<InputManager>();

            protobufSocket = new ProtobufSocket(gameConfiguration.ServerAddress, (int)playerSelfInfo.GameServerPort, false);

            jsonFormatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));

            clientMessageSequenceNum = 0;
            serverMessageSequenceNum = 0;

            // Prepare commmon messages
            playerIdentity = new PlayerIdentity
            {
                PlayerId = playerSelfInfo.PlayerID,
                RoomId = playerSelfInfo.RoomID
            };

            playerInput = new PlayerInput
            {
                PlayerIdentity = playerIdentity
            };

            connectionPong = new ConnectionPong()
            {
                PlayerIdentity = playerIdentity
            };

            winnerIDs = new List<uint>();
        }


        void Start()
        {
            errorOccured = false;
            StartCoroutine(ExecuteConnectionSequence());
        }

        private IEnumerator ExecuteConnectionSequence()
        {
            state = ConnectionState.Connecting;
            JoinGameRequest joinGameRequest = new JoinGameRequest()
            {
                PlayerIdentity = playerIdentity
            };

            // TODO: Add timeout
            while (state == ConnectionState.Connecting)
            {
                protobufSocket.SendProtoMessage((byte)MessageType.JoinGameRequest, joinGameRequest);
                LogProtoMessage(MessageType.JoinGameRequest, joinGameRequest, false);

                while (protobufSocket.ReceiveProtoMessage(out byte messageType, out ReadOnlySequence<byte> messageSpan))
                {
                    if ((MessageType)messageType == MessageType.JoinGameResponse)
                    {
                        JoinGameResponse joinGameResponse = JoinGameResponse.Parser.ParseFrom(messageSpan);

                        LogProtoMessage(MessageType.JoinGameResponse, joinGameResponse);

                        if (!joinGameResponse.Success)
                        {
                            state = ConnectionState.Disconnected;
                            errorOccured = true;
                            yield break;
                        }
                        else
                        {
                            // Setup game world
                            MapInfo mapInfo = joinGameResponse.MapInfo;
                            Vector2Int[] playerSpawnPositions = new Vector2Int[mapInfo.PlayerSpawnPositions.Count];
                            for (int i = 0; i < playerSpawnPositions.Length; i++)
                            {
                                MapInfo.Types.PlayerTilePosition spawnPosition = mapInfo.PlayerSpawnPositions[i];
                                playerSpawnPositions[spawnPosition.PlayerId] = new Vector2Int((int)spawnPosition.X, (int)spawnPosition.Y);
                            }
                            gameWorldController.SetupGameWorld((int)mapInfo.Width, (int)mapInfo.Height, mapInfo.MapData.Span, playerSpawnPositions);

                            // Send map received message
                            MapInfoReceived mapInfoReceived = new MapInfoReceived
                            {
                                PlayerIdentity = playerIdentity
                            };
                            protobufSocket.SendProtoMessage((byte)MessageType.MapInfoReceived, mapInfoReceived);

                            LogProtoMessage(MessageType.MapInfoReceived, mapInfoReceived, false);

                            // Change state
                            state = ConnectionState.Connected;
                            break;
                        }
                    }
                }

                yield return new WaitForSecondsRealtime(1.0f / 10.0f);
            }

            // TODO: Add timeout
            while (state == ConnectionState.Connected)
            {
                while (protobufSocket.ReceiveProtoMessage(out byte messageType, out ReadOnlySequence<byte> messageSpan))
                {
                    if ((MessageType)messageType == MessageType.GameStatusChanged)
                    {
                        GameStatusChanged gameStatusChanged = GameStatusChanged.Parser.ParseFrom(messageSpan);

                        LogProtoMessage(MessageType.GameStatusChanged, gameStatusChanged);

                        switch (gameStatusChanged.Status)
                        {
                            case GameStatusChanged.Types.Status.Error:
                                state = ConnectionState.Disconnected;
                                errorOccured = true;
                                yield break;
                            case GameStatusChanged.Types.Status.Starting:
                                gameWorldController.RegisterPlayers(gameStatusChanged.StartingInfo.ConnectedPlayerIds);
                                break;
                            case GameStatusChanged.Types.Status.CountingDown:
                                break;
                            case GameStatusChanged.Types.Status.Started:
                                state = ConnectionState.Playing;
                                break;
                            default:
                                break;
                        }
                    }
                }

                yield return new WaitForSecondsRealtime(1.0f / 10.0f);
            }

            inputManager.enabled = true;
            canSendInput = true;
            while (state == ConnectionState.Playing)
            {
                PollGameServerMessages();
                if (canSendInput)
                {
                    SendInput();
                }

                yield return new WaitForSecondsRealtime(1.0f / 40.0f);
            }

            StartCoroutine(OnGameExit());
        }

        void PollGameServerMessages()
        {
            while (protobufSocket.ReceiveProtoMessage(out byte messageTypeByte, out ReadOnlySequence<byte> messageSpan))
            {
                MessageType messageType = (MessageType)messageTypeByte;
                switch (messageType)
                {
                    case MessageType.GameState:
                        GameState gameState = GameState.Parser.ParseFrom(messageSpan);

                        // @@@@@@@@@@@@@@@@@@
                        LogProtoMessage(MessageType.GameState, gameState);

                        // Check sequence number
                        if (gameState.SequenceNum <= serverMessageSequenceNum)
                        {
                            break;
                        }
                        serverMessageSequenceNum = gameState.SequenceNum;

                        UpdateGameState(gameState);
                        break;
                    case MessageType.GameStatusChanged:
                        GameStatusChanged gameStatusChanged = GameStatusChanged.Parser.ParseFrom(messageSpan);

                        LogProtoMessage(MessageType.GameStatusChanged, gameStatusChanged);

                        if (gameStatusChanged.Status == GameStatusChanged.Types.Status.Error)
                        {
                            state = ConnectionState.Disconnected;
                            errorOccured = true;
                        }
                        else if (gameStatusChanged.Status == GameStatusChanged.Types.Status.Ended)
                        {
                            state = ConnectionState.Disconnected;

                            var endInfo = gameStatusChanged.EndInfo;
                            winnerIDs.AddRange(endInfo.WinnerIds);
                        }

                        break;
                    case MessageType.ConnectionPing:
                        ConnectionPing connectionPing = ConnectionPing.Parser.ParseFrom(messageSpan);
                        protobufSocket.SendProtoMessage((byte)MessageType.ConnectionPong, connectionPong);
                        break;
                    default: 
                        break;
                }
            }
        }

        private void UpdateGameState(GameState gameState)
        {
            gameOverlayUIController.SetGameTime(gameState.GameTimer);

            gameWorldController.UpdateGameState(gameState);
            if (canSendInput && gameWorldController.SelfState() == PlayerController.PlayerState.Dead)
            {
                canSendInput = false;
            }
        }

        void SendInput()
        {
            playerInput.SequenceNum = ++clientMessageSequenceNum;
            playerInput.DirectionX = inputManager.Direction.x;
            playerInput.DirectionY = inputManager.Direction.y;
            playerInput.BombPlaced = inputManager.BombPlaced;
            inputManager.BombPlaced = false;

            protobufSocket.SendProtoMessage((byte)MessageType.PlayerInput, playerInput);
        }

        IEnumerator OnGameExit()
        {
            if (!errorOccured)
            {
                ShowEndScreen();
                yield return new WaitForSecondsRealtime(2.0f);
            }

            UnityEngine.SceneManagement.SceneManager.LoadScene("Scenes/EntranceScene");
        }

        void ShowEndScreen()
        {
            gameEndUIController.SetUIEnabled(true);
            gameEndUIController.SetWinners(winnerIDs);
        }

        private void LogProtoMessage(MessageType messageType, IMessage message, bool isReceived = true)
        {
            string messageString = message == null ? "" : jsonFormatter.Format(message);
            string side = isReceived ? "Received" : "Sending";
            Debug.Log($"{side} {messageType}\n{messageString}");
        }
    }
}