using System;
using UnityEngine;
using UnityEngine.UIElements;

namespace BoomOnline2.UI
{
    public class LoginUIController : UIControllerBase
    {
        public enum ConnectionState
        {
            Invalid,
            Valid,
            Waiting
        }

        TextField serverAddressText;
        Button testConnectionButton;

        TextField playerNameText;
        Button playGameButton;

        private Texture2D validIcon;
        private Texture2D invalidIcon;
        private Texture2D refreshIcon;

        ConnectionState connectionState;

        public string ServerAddress => serverAddressText?.value;

        public event Action TestConnection;
        public event Action Login;

        protected override void Awake()
        {
            base.Awake();

            validIcon = Resources.Load<Texture2D>("UITexture/valid");
            invalidIcon = Resources.Load<Texture2D>("UITexture/invalid");
            refreshIcon = Resources.Load<Texture2D>("UITexture/refresh2");

            if (validIcon == null || invalidIcon == null || refreshIcon == null)
            {
                Debug.LogError("Cannot find icon(s) for LoginScreen", this);
            }
        }

        private void OnEnable()
        {
            UIDocument loginUIDocument = uiDocument;
            VisualElement documentRoot = loginUIDocument.rootVisualElement;

            serverAddressText = documentRoot.Q<TextField>("text-serverAddress");
            RegisterCallbackSmart<KeyUpEvent>(serverAddressText, OnServerAddressKeyUp);

            testConnectionButton = documentRoot.Q<Button>("button-testConnection");
            RegisterCallbackSmart<ClickEvent>(testConnectionButton, OnTestConnectionClicked);

            playerNameText = documentRoot.Q<TextField>("text-playerName");
            RegisterCallbackSmart<KeyUpEvent>(playerNameText, OnPlayerNameKeyUp);

            playGameButton = documentRoot.Q<Button>("button-playGame");
            playGameButton.SetEnabled(false);
            RegisterCallbackSmart<ClickEvent>(playGameButton, OnPlayGameClicked);

            Button exitGameButton = documentRoot.Q<Button>("button-exitGame");
            RegisterCallbackSmart<ClickEvent>(exitGameButton, OnExitGameClicked);

            SetConnectionState(ConnectionState.Invalid);
        }

        protected override void OnDisable()
        {
            base.OnDisable();
        }
        public void SetConnectionState(ConnectionState state)
        {
            connectionState = state;
            switch (state)
            {
                case ConnectionState.Invalid:
                    testConnectionButton.style.backgroundImage = new StyleBackground(invalidIcon);
                    testConnectionButton.style.unityBackgroundImageTintColor = Color.red;
                    testConnectionButton.SetEnabled(true);
                    serverAddressText.isReadOnly = false;
                    break;
                case ConnectionState.Valid:
                    testConnectionButton.style.backgroundImage = new StyleBackground(validIcon);
                    testConnectionButton.style.unityBackgroundImageTintColor = Color.green;
                    testConnectionButton.SetEnabled(true);
                    serverAddressText.isReadOnly = false;
                    break;
                case ConnectionState.Waiting:
                    testConnectionButton.style.backgroundImage = new StyleBackground(refreshIcon);
                    testConnectionButton.style.unityBackgroundImageTintColor = Color.black;
                    testConnectionButton.SetEnabled(false);
                    serverAddressText.isReadOnly = true;
                    break;
                default:
                    break;
            }

            ValidateLogin();
        }

        private void ValidateLogin()
        {
            if (connectionState == ConnectionState.Valid && playerNameText.value.Length > 0)
            {
                playGameButton.SetEnabled(true);
            }
            else
            {
                playGameButton.SetEnabled(false);
            }
        }

        private void OnTestConnectionClicked(ClickEvent clickEvent)
        {
            TestConnection?.Invoke();
        }

        private void OnServerAddressKeyUp(KeyUpEvent keyUpEvent)
        {
        }

        private void OnPlayerNameKeyUp(KeyUpEvent keyUpEvent)
        {
            ValidateLogin();
        }

        private void OnPlayGameClicked(ClickEvent clickEvent)
        {
            PlayerSelfInfo.Instance.Name = playerNameText.value;

            Login?.Invoke();
        }

        private void OnExitGameClicked(ClickEvent clickEvent)
        {
            Application.Quit();
        }
    }
}