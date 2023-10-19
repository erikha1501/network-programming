using BoomOnline2.GamePlay;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

namespace BoomOnline2.UI
{
    class GameEndUIController : UIControllerBase
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

        PlayerCard[] playerCardList;

        protected override void Awake()
        {
            base.Awake();
        }
        private void OnEnable()
        {
            UIDocument roomUIController = uiDocument;
            VisualElement documentRoot = roomUIController.rootVisualElement;

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
        }

        public void SetWinners(List<uint> winnerIDs)
        {
            for (int i = 0; i < playerCardList.Length; i++)
            {
                Player player = PlayerSelfInfo.Instance.Room[i];
                if (player != null)
                {
                    playerCardList[i].PlayerName.text = player.Name;
                }
                else
                {
                    playerCardList[i].PlayerName.text = $"Player #{i}";
                }

                playerCardList[i].PlayerIcon.style.unityBackgroundImageTintColor = Color.black;
            }

            foreach (uint winnerID in winnerIDs)
            {
                playerCardList[winnerID].PlayerName.text = PlayerSelfInfo.Instance.Room[(int)winnerID].Name;
                playerCardList[winnerID].PlayerIcon.style.unityBackgroundImageTintColor = Color.white;
            }
        }

        protected override void OnDisable()
        {
            base.OnDisable();
        }
    }
}