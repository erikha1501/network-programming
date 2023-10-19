using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

namespace BoomOnline2.UI
{
    class RoomListView : CustomListView<Room, RoomViewHolder>
    {
        new class UxmlFactory : UxmlFactory<RoomListView, UxmlTraits> { }

        new class UxmlTraits : CustomListView<Room, RoomViewHolder>.UxmlTraits { }

        protected override void OnBindItem(RoomViewHolder viewHolder, Room item)
        {
            viewHolder.Update(item);
        }
    }

    class RoomViewHolder : ViewHolder
    {
        public Label RoomIDLabel { get; private set; }
        public Label RoomNameLabel { get; private set; }
        public Label RoomPlayerCountLabel { get; private set; }

        public override void OnCreate(VisualElement visualElement)
        {
            RoomIDLabel = visualElement.Q<Label>("label-roomID");
            RoomNameLabel = visualElement.Q<Label>("label-roomName");
            RoomPlayerCountLabel = visualElement.Q<Label>("label-roomPlayerCount");
        }

        public void Update(Room room)
        {
            RoomIDLabel.text = room.ID.ToString();

            Player roomOwner = room.GetOwner();
            if (roomOwner != null)
            {
                RoomNameLabel.text = $"{roomOwner.Name}'s room";
            }
            else
            {
                RoomNameLabel.text = "Error";
            }

            RoomPlayerCountLabel.text = $"{room.Players.Count}/4";

            Color color = room.Players.Count switch
            {
                4 => new Color32(229, 112, 126, 255),
                _ => new Color32(0, 210, 39, 255)
            };

            RoomPlayerCountLabel.style.color = new StyleColor(color);
        }
    } 
}