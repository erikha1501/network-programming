using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

namespace BoomOnline2.UI
{
    class LobbyUIController : UIControllerBase
    {
        private RoomListView roomListView;

        public event Action RefreshRoomList;
        public event Action CreateRoom;
        public event Action<Room> JoinRoom;

        protected override void Awake()
        {
            base.Awake();
        }
        private void OnEnable()
        {
            UIDocument lobbyUIDocument = uiDocument;
            VisualElement documentRoot = lobbyUIDocument.rootVisualElement;

            // Greeting label
            Label greetingLabel = documentRoot.Q<Label>("label-playerGreeting");
            greetingLabel.text = $"Hello {PlayerSelfInfo.Instance.Name}";

            // Create room button
            Button createRoomButton = documentRoot.Q<Button>("button-createRoom");
            RegisterCallbackSmart<ClickEvent>(createRoomButton, OnCreateRoomClicked);

            // Refresh room list button
            Button refreshButton = documentRoot.Q<Button>("button-refreshRoomList");
            RegisterCallbackSmart<ClickEvent>(refreshButton, OnRefreshRoomListClicked);

            // List view
            roomListView = documentRoot.Q<RoomListView>("list-rooms");
            RegisterCallbackSmart<ClickEvent>(roomListView, OnRoomSelected);
        }

        protected override void OnDisable()
        {
            base.OnDisable();
        }

        public void UpdateRoomList(IList rooms)
        {
            roomListView.itemsSource = rooms;
        }

        private void OnRefreshRoomListClicked(ClickEvent clickEvent)
        {
            RefreshRoomList?.Invoke();
        }

        private void OnCreateRoomClicked(ClickEvent clickEvent)
        {
            CreateRoom?.Invoke();
        }

        private void OnRoomSelected(ClickEvent clickEvent)
        {
            if (clickEvent.clickCount == 2)
            {
                JoinRoom?.Invoke((Room)roomListView.selectedItem);
            }
        }
    }
}