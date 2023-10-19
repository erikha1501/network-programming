using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

namespace BoomOnline2.UI
{
    public abstract class UIControllerBase : MonoBehaviour
    {
        private List<Action> callbackUnregisterActionList;

        protected UIDocument uiDocument;

        protected void RegisterCallbackSmart<TEventType>(CallbackEventHandler targetHandler, EventCallback<TEventType> callback) where TEventType : EventBase<TEventType>, new()
        {
            targetHandler.RegisterCallback(callback);
            callbackUnregisterActionList.Add(() => targetHandler.UnregisterCallback(callback));
        }

        protected virtual void Awake()
        {
            callbackUnregisterActionList = new List<Action>();

            uiDocument = GetComponent<UIDocument>();

            if (uiDocument is null)
            {
                Debug.LogWarning("No UIDocument attached to this object", this);
            }
        }

        protected virtual void OnDisable()
        {
            foreach (var unregisterAction in callbackUnregisterActionList)
            {
                unregisterAction();
            }
            callbackUnregisterActionList.Clear();
        }

        public void SetUIEnabled(bool enabled)
        {
            gameObject.SetActive(enabled);
        }
    }
}