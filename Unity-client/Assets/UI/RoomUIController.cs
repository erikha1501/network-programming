using BoomOnline2.GamePlay;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

namespace BoomOnline2.UI
{
    class RoomUIController : UIControllerBase
    {
        struct PlayerCard
        {
            public VisualElement PlayerIcon { get; }
            public Label PlayerName { get; }

            public PlayerCard(VisualElement playerIcon, Label playerName)
            {
                PlayerIcon = playerIcon;
                PlayerName = playerName;
            }
        }

        public event Action LeaveRoom;
        public event Action StartGame;

        Label roomIDLabel;
        Label roomNameLabel;
        PlayerCard[] playerCardList;

        Button startGameButton;

        protected override void Awake()
        {
            base.Awake();
        }
        private void OnEnable()
        {
            UIDocument roomUIController = uiDocument;
            VisualElement documentRoot = roomUIController.rootVisualElement;

            // Room id label
            roomIDLabel = documentRoot.Q<Label>("label-roomID");

            // Room name label
            roomNameLabel = documentRoot.Q<Label>("label-roomName");

            playerCardList = new PlayerCard[4];

            VisualElement playerContainer = documentRoot.Q<VisualElement>("playerContainer");

            if (playerContainer.childCount != 4)
            {
                Debug.Log("Cannot find 4 player cards in Room screen", this);
                return;
            }

            for (int i = 0; i < playerCardList.Length; i++)
            {
                VisualElement playerCardElement = playerContainer[i];

                VisualElement playerIconElement = playerCardElement.Q<VisualElement>(null, "player-icon");
                Label playerNameLabel = playerCardElement.Q<Label>(null, "player-name");

                playerCardList[i] = new PlayerCard(playerIconElement, playerNameLabel);
            }

            // Start game button
            startGameButton = documentRoot.Q<Button>("button-startGame");
            RegisterCallbackSmart<ClickEvent>(startGameButton, OnStartGameClicked);

            // Leave room button
            Button leaveRoomButton = documentRoot.Q<Button>("button-leaveRoom");
            RegisterCallbackSmart<ClickEvent>(leaveRoomButton, OnLeaveRoomClicked);
        }

        public void SetRoom(Room room)
        {
            roomIDLabel.text = room.ID.ToString();

            Player roomOwner = room.GetOwner();

            if (roomOwner != null)
            {
                roomNameLabel.text = $"{roomOwner.Name}'s room";
            }
            else
            {
                roomNameLabel.text = "Error";
            }

            for (int i = 0; i < playerCardList.Length; i++)
            {
                playerCardList[i].PlayerName.text = $"Player #{i}";
                playerCardList[i].PlayerIcon.style.unityBackgroundImageTintColor = Color.black;
            }

            foreach (var player in room.Players)
            {
                playerCardList[player.ID].PlayerName.text = player.Name;
                playerCardList[player.ID].PlayerIcon.style.unityBackgroundImageTintColor = Color.white;
            }

            startGameButton.SetEnabled(PlayerSelfInfo.Instance.PlayerID == room.OwnerID && room.Players.Count >= GameConstant.MinPlayerCount);
        }

        protected override void OnDisable()
        {
            base.OnDisable();
        }

        private void OnStartGameClicked(ClickEvent clickEvent)
        {
            StartGame?.Invoke();
        }

        private void OnLeaveRoomClicked(ClickEvent clickEvent)
        {
            LeaveRoom?.Invoke();
        }
    }
}