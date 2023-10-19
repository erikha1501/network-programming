using BoomOnline2.GamePlay;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

namespace BoomOnline2.UI
{
    class GameOverlayUIController : UIControllerBase
    {
        Label gameTimeLabel;

        protected override void Awake()
        {
            base.Awake();
        }
        private void OnEnable()
        {
            UIDocument roomUIController = uiDocument;
            VisualElement documentRoot = roomUIController.rootVisualElement;

            gameTimeLabel = documentRoot.Q<Label>("label-gameTime");
        }

        public void SetGameTime(float gameTime)
        {
            TimeSpan result = TimeSpan.FromSeconds(gameTime);
            gameTimeLabel.text = result.ToString(@"mm\:ss");
        }

        protected override void OnDisable()
        {
            base.OnDisable();
        }
    }
}