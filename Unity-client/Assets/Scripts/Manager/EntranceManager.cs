using System.Buffers;
using System.Linq;
using System.Collections.Generic;

using UnityEngine;

using Google.Protobuf.Collections;
using Google.Protobuf;
using Pnet;

using BoomOnline2.UI;
using System.Net;
using System.Net.Sockets;

namespace BoomOnline2
{
    class EntranceManager : MonoBehaviour
    {
        [Header("Screens")]
        [SerializeField]
        private LoginUIController loginUIController;
        [SerializeField]
        private LobbyUIController lobbyUIController;
        [SerializeField]
        private RoomUIController roomUIController;

        [Space]

        private ProtobufSocket roomProtobufSocket;
        private JsonFormatter jsonFormatter;

        PlayerSelfInfo playerSelfInfo;
        GameConfiguration gameConfiguration;

        private List<Room> roomList;

        private void Awake()
        {
            gameConfiguration = GameConfiguration.Instance;
            playerSelfInfo = PlayerSelfInfo.Instance;

            roomProtobufSocket = gameConfiguration.RoomProtobufSocket;

            jsonFormatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));

            if (playerSelfInfo.Name == null)
            {
                ChangeStatus(PlayerSelfInfo.Status.InLogin);
            }
            else
            {
                if (playerSelfInfo.PlayerStatus != PlayerSelfInfo.Status.InGame)
                {
                    Debug.Log("Invalid state");
                    Application.Quit();
                }

                ChangeStatus(PlayerSelfInfo.Status.InRoom);
                ReenterRoom();
            }

            roomList = new List<Room>();

            // Setup screen
            Screen.SetResolution(720, 720, false);
        }

        private void OnDisable()
        {
            UnregisterAllEvents();
        }

        private void UnregisterAllEvents()
        {
            loginUIController.TestConnection -= TestServerConnection;
            loginUIController.Login -= PlayerLogin;

            lobbyUIController.RefreshRoomList -= RefreshRoomList;
            lobbyUIController.CreateRoom -= CreateRoom;
            lobbyUIController.JoinRoom -= JoinRoom;

            roomUIController.LeaveRoom -= LeaveRoom;
            roomUIController.StartGame -= StartGame;
        }

        private void ChangeStatus(PlayerSelfInfo.Status status)
        {
            playerSelfInfo.PlayerStatus = status;
            UnregisterAllEvents();
            StopAllCoroutines();
            switch (status)
            {
                case PlayerSelfInfo.Status.InLogin:
                    StartCoroutine(PollServerLoginMessagesCoroutine());
                    loginUIController.SetUIEnabled(true);
                    lobbyUIController.SetUIEnabled(false);
                    roomUIController.SetUIEnabled(false);

                    loginUIController.TestConnection += TestServerConnection;
                    loginUIController.Login += PlayerLogin;

                    TestServerConnection();
                    break;
                case PlayerSelfInfo.Status.InLobby:
                    StartCoroutine(PollServerLobbyMessagesCoroutine());
                    loginUIController.SetUIEnabled(false);
                    lobbyUIController.SetUIEnabled(true);
                    roomUIController.SetUIEnabled(false);

                    lobbyUIController.RefreshRoomList += RefreshRoomList;
                    lobbyUIController.CreateRoom += CreateRoom;
                    lobbyUIController.JoinRoom += JoinRoom;
                    break;
                case PlayerSelfInfo.Status.InRoom:
                    StartCoroutine(PollServerRoomMessagesCoroutine());
                    loginUIController.SetUIEnabled(false);
                    lobbyUIController.SetUIEnabled(false);
                    roomUIController.SetUIEnabled(true);

                    roomUIController.LeaveRoom += LeaveRoom;
                    roomUIController.StartGame += StartGame;
                    break;
                default:
                    break;
            }
        }

        private void TestServerConnection()
        {
            // Validate server address
            if (IPAddress.TryParse(loginUIController.ServerAddress, out IPAddress newServerAddress))
            {
                gameConfiguration.UpdateServerEndpoint(newServerAddress);
                StartCoroutine(nameof(TestServerConnectionRoutine));
            }
            else
            {
                Debug.Log("Invalid address");
            }
        }

        private System.Collections.IEnumerator TestServerConnectionRoutine()
        {
            ServerConnectionPing serverConnectionPing = new ServerConnectionPing();
            roomProtobufSocket.SendProtoMessage((byte)MessageType.ServerConnectionPing, serverConnectionPing);
            loginUIController.SetConnectionState(LoginUIController.ConnectionState.Waiting);

            yield return new WaitForSecondsRealtime(3.0f);

            loginUIController.SetConnectionState(LoginUIController.ConnectionState.Invalid);
        }

        private void PlayerLogin()
        {
            ChangeStatus(PlayerSelfInfo.Status.InLobby);
        }

        private void RefreshRoomList()
        {
            QueryRoomInfoRequest queryRoomInfoRequest = new QueryRoomInfoRequest();
            roomProtobufSocket.SendProtoMessage((byte)MessageType.QueryRoomInfoRequest, queryRoomInfoRequest);

            LogProtoMessage(MessageType.QueryRoomInfoRequest, queryRoomInfoRequest, false);
        }

        private void CreateRoom()
        {
            CreateRoomRequest createRoomRequest = new CreateRoomRequest()
            {
                ClientInfo = new ClientInfo
                {
                    Name = playerSelfInfo.Name
                }
            };

            roomProtobufSocket.SendProtoMessage((byte)MessageType.CreateRoomRequest, createRoomRequest);

            LogProtoMessage(MessageType.CreateRoomRequest, createRoomRequest, false);
        }

        private void JoinRoom(Room room)
        {
            JoinRoomRequest joinRoomRequest = new JoinRoomRequest()
            {
                ClientInfo = new ClientInfo
                {
                    Name = playerSelfInfo.Name
                },
                RoomId = room.ID
            };

            roomProtobufSocket.SendProtoMessage((byte)MessageType.JoinRoomRequest, joinRoomRequest);

            LogProtoMessage(MessageType.JoinRoomRequest, joinRoomRequest, false);
        }

        private void ReenterRoom()
        {
            QuerySelfRoomInfoRequest querySelfRoomInfoRequest = new QuerySelfRoomInfoRequest
            {
                ClientIdentity = new ClientIdentity
                {
                    ClientId = playerSelfInfo.PlayerID,
                    RoomId = playerSelfInfo.RoomID
                }
            };

            roomProtobufSocket.SendProtoMessage((byte)MessageType.QuerySelfRoomInfoRequest, querySelfRoomInfoRequest);
        }

        private void LeaveRoom()
        {
            LeaveRoomRequest leaveRoomRequest = new LeaveRoomRequest()
            {
                ClientIdentity = new ClientIdentity
                {
                    ClientId = playerSelfInfo.PlayerID,
                    RoomId = playerSelfInfo.RoomID
                }
            };

            roomProtobufSocket.SendProtoMessage((byte)MessageType.LeaveRoomRequest, leaveRoomRequest);

            LogProtoMessage(MessageType.LeaveRoomRequest, leaveRoomRequest, false);
        }

        private void StartGame()
        {
            StartGameRequest startGameRequest = new StartGameRequest()
            {
                ClientIdentity = new ClientIdentity
                {
                    ClientId = playerSelfInfo.PlayerID,
                    RoomId = playerSelfInfo.RoomID
                }
            };

            roomProtobufSocket.SendProtoMessage((byte)MessageType.StartGameRequest, startGameRequest);

            LogProtoMessage(MessageType.StartGameRequest, startGameRequest, false);
        }

        private System.Collections.IEnumerator PollServerLoginMessagesCoroutine()
        {
            yield return null;
            while (true)
            {
                try
                {
                    while (roomProtobufSocket.ReceiveProtoMessage(out byte messageTypeByte, out ReadOnlySequence<byte> messageSpan))
                    {
                        MessageType messageType = (MessageType)messageTypeByte;
                        switch (messageType)
                        {
                            case MessageType.ServerConnectionPong:
                                ServerConnectionPong serverConnectionPong = ServerConnectionPong.Parser.ParseFrom(messageSpan);
                                LogProtoMessage(messageType, serverConnectionPong);
                                OnServerConnectionPong();
                                break;
                            default:
                                break;
                        }
                    }
                }
                catch (SocketException)
                {
                    // If the remote host does not have a listener on target port, UDP socket may
                    // throw an exception on Recv.
                    // Since this coroutine is only used for connection test,
                    // ignoring socket exception here is OK.
                }

                yield return new WaitForSecondsRealtime(1.0f / 2.0f);
            }
        }

        private System.Collections.IEnumerator PollServerLobbyMessagesCoroutine()
        {
            yield return null;
            while (true)
            {
                while (roomProtobufSocket.ReceiveProtoMessage(out byte messageTypeByte, out ReadOnlySequence<byte> messageSpan))
                {
                    MessageType messageType = (MessageType)messageTypeByte;
                    switch (messageType)
                    {
                        case MessageType.QueryRoomInfoResponse:
                            QueryRoomInfoResponse queryRoomInfoResponse = QueryRoomInfoResponse.Parser.ParseFrom(messageSpan);
                            LogProtoMessage(messageType, queryRoomInfoResponse);
                            OnRoomListUpdated(queryRoomInfoResponse.Rooms);
                            break;
                        case MessageType.CreateRoomResponse:
                            CreateRoomResponse createRoomResponse = CreateRoomResponse.Parser.ParseFrom(messageSpan);
                            LogProtoMessage(messageType, createRoomResponse);

                            if (createRoomResponse.Success)
                            {
                                OnRoomJoined(createRoomResponse.AssignedIdentity, new Room(createRoomResponse.RoomInfo));
                            }
                            break;
                        case MessageType.JoinRoomResponse:
                            JoinRoomResponse joinRoomResponse = JoinRoomResponse.Parser.ParseFrom(messageSpan);
                            LogProtoMessage(messageType, joinRoomResponse);

                            if (joinRoomResponse.Success)
                            {
                                OnRoomJoined(joinRoomResponse.AssignedIdentity, new Room(joinRoomResponse.RoomInfo));
                            }
                            break;
                        default:
                            break;
                    }
                }

                yield return new WaitForSecondsRealtime(1.0f / 2.0f);
            }
        }

        private System.Collections.IEnumerator PollServerRoomMessagesCoroutine()
        {
            yield return null;
            while (true)
            {
                while (roomProtobufSocket.ReceiveProtoMessage(out byte messageTypeByte, out ReadOnlySequence<byte> messageSpan))
                {
                    MessageType messageType = (MessageType)messageTypeByte;
                    switch (messageType)
                    {
                        case MessageType.ConnectionPing:
                            LogProtoMessage(messageType, null);
                            ConnectionPong connectionPong = new ConnectionPong
                            {
                                ClientIdentity = new ClientIdentity
                                {
                                    ClientId = playerSelfInfo.PlayerID,
                                    RoomId = playerSelfInfo.RoomID
                                }
                            };
                            roomProtobufSocket.SendProtoMessage((byte)MessageType.ConnectionPong, connectionPong);
                            break;
                        case MessageType.LeaveRoomResponse:
                            LeaveRoomResponse leaveRoomResponse = LeaveRoomResponse.Parser.ParseFrom(messageSpan);
                            LogProtoMessage(messageType, leaveRoomResponse);

                            if (leaveRoomResponse.Success)
                            {
                                playerSelfInfo.Room = null;
                                ChangeStatus(PlayerSelfInfo.Status.InLobby);
                            }
                            break;
                        case MessageType.RoomInfoChanged:
                            RoomInfoChanged roomInfoChanged = RoomInfoChanged.Parser.ParseFrom(messageSpan);
                            LogProtoMessage(messageType, roomInfoChanged);

                            UpdateRoom(new Room(roomInfoChanged.NewRoomInfo));
                            break;
                        case MessageType.StartGameResponse:
                            StartGameResponse startGameResponse = StartGameResponse.Parser.ParseFrom(messageSpan);
                            LogProtoMessage(messageType, startGameResponse);
                            break;
                        case MessageType.GameCreated:
                            GameCreated gameCreated = GameCreated.Parser.ParseFrom(messageSpan);
                            LogProtoMessage(messageType, gameCreated);
                            OnGameCreated(gameCreated);
                            break;
                        default:
                            break;
                    }
                }

                yield return new WaitForSecondsRealtime(1.0f / 2.0f);
            }
        }

        private void OnServerConnectionPong()
        {
            StopCoroutine(nameof(TestServerConnectionRoutine));
            loginUIController.SetConnectionState(LoginUIController.ConnectionState.Valid);
        }

        private void OnRoomListUpdated(RepeatedField<RoomInfo> roomInfos)
        {
            roomList.Clear();
            roomList.AddRange(roomInfos.Select(roomInfo => new Room(roomInfo)));
            lobbyUIController.UpdateRoomList(roomList);
        }

        private void OnRoomJoined(ClientIdentity assignedIdentity, Room room)
        {
            playerSelfInfo.PlayerID = assignedIdentity.ClientId;
            playerSelfInfo.RoomID = assignedIdentity.RoomId;

            ChangeStatus(PlayerSelfInfo.Status.InRoom);
            UpdateRoom(room);
        }

        private void OnGameCreated(GameCreated gameCreated)
        {
            playerSelfInfo.GameServerPort = gameCreated.Port;
            playerSelfInfo.PlayerStatus = PlayerSelfInfo.Status.InGame;

            UnityEngine.SceneManagement.SceneManager.LoadScene("Scenes/GameScene");
        }

        private void UpdateRoom(Room room)
        {
            playerSelfInfo.Room = room;
            roomUIController.SetRoom(room);
        }

        private void LogProtoMessage(MessageType messageType, IMessage message, bool isReceived = true)
        {
            string messageString = message == null ? "" : jsonFormatter.Format(message);
            string side = isReceived ? "Received" : "Sending";
            Debug.Log($"{side} {messageType}\n{messageString}");
        }
    }
}
