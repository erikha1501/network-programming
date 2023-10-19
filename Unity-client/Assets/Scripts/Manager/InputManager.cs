using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace BoomOnline2
{
    public class InputManager : MonoBehaviour
    {
        public Vector2 Direction { get; set; }
        public bool BombPlaced { get; set; }

        private Vector2 prioritizedDirection;

        void Update()
        {
            Vector2 inputDirection = Vector2.zero;

            // Prioritized input
            if (Input.GetKeyDown(KeyCode.UpArrow))
            {
                prioritizedDirection.y = Vector2.up.y;
            }
            if (Input.GetKeyDown(KeyCode.DownArrow))
            {
                prioritizedDirection.y = Vector2.down.y;
            }
            if (Input.GetKeyDown(KeyCode.LeftArrow))
            {
                prioritizedDirection.x = Vector2.left.x;
            }
            if (Input.GetKeyDown(KeyCode.RightArrow))
            {
                prioritizedDirection.x = Vector2.right.x;
            }

            // Persistent input
            if (Input.GetKey(KeyCode.UpArrow))
            {
                inputDirection += Vector2.up;
            }
            if (Input.GetKey(KeyCode.DownArrow))
            {
                inputDirection += Vector2.down;

                if (inputDirection.y == 0.0f)
                {
                    inputDirection.y = prioritizedDirection.y;
                }
            }
            if (Input.GetKey(KeyCode.LeftArrow))
            {
                inputDirection += Vector2.left;
            }
            if (Input.GetKey(KeyCode.RightArrow))
            {
                inputDirection += Vector2.right;

                if (inputDirection.x == 0.0f)
                {
                    inputDirection.x = prioritizedDirection.x;
                }
            }


            Direction = inputDirection;
            if (Input.GetKeyDown(KeyCode.Space))
            {
                BombPlaced = true;
            }
        }
    }
}