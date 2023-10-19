using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

namespace BoomOnline2.UI
{
    internal abstract class CustomListView<TItem, TViewHolder> : ListView where TViewHolder : ViewHolder, new()
    {
        internal new class UxmlTraits : ListView.UxmlTraits
        {
            private readonly UxmlStringAttributeDescription m_templatePath = new UxmlStringAttributeDescription { name = "template-path" };

            public override void Init(VisualElement ve, IUxmlAttributes bag, CreationContext cc)
            {
                base.Init(ve, bag, cc);
                CustomListView<TItem, TViewHolder> item = ve as CustomListView<TItem, TViewHolder>;

                item.TemplatePath = m_templatePath.GetValueFromBag(bag, cc);

                item.Init();
            }
        }

        public string TemplatePath { get; private set; }

        protected VisualTreeAsset templateTreeAsset;

        public CustomListView()
        {
        }

        protected virtual void Init()
        {
            templateTreeAsset = Resources.Load<VisualTreeAsset>(TemplatePath);

            makeItem = OnCreateItem;
            bindItem = (visualElement, index) => OnBindItem((TViewHolder)visualElement.userData, (TItem)itemsSource[index]);
        }

        protected virtual VisualElement OnCreateItem()
        {
            VisualElement visualElement = templateTreeAsset.CloneTree();
            TViewHolder viewHolder = new TViewHolder();
            viewHolder.OnCreate(visualElement);
            visualElement.userData = viewHolder;

            return visualElement;
        }

        protected abstract void OnBindItem(TViewHolder viewHolder, TItem item);
    }

    abstract class ViewHolder
    {
        public abstract void OnCreate(VisualElement visualElement);
    }
}