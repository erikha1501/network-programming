using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

namespace BoomOnline2
{
    public class PlayerController : MonoBehaviour
    {
        public enum PlayerState
        {
            Dead,
            Alive
        }

        [SerializeField]
        private TextMeshPro nameTextMesh;
        [SerializeField]
        private Transform graphicsTransform;
        [SerializeField]
        private Animator graphicsAnimator;

        [Space]
        [SerializeField]
        private GameObject disappearAnimationPrefab;

        float initialXScale;

        public PlayerState State { get; private set; }

        private void Awake()
        {
            if (nameTextMesh == null)
            {
                Debug.LogError("Missing name plate component", this.gameObject);
            }

            if (graphicsTransform == null)
            {
                Debug.LogError("Missing graphics transform component", this.gameObject);
            }

            if (graphicsAnimator == null)
            {
                Debug.LogError("Missing graphics animator component", this.gameObject);
            }

            initialXScale = graphicsTransform.localScale.x;
        }

        private void Start()
        {
            State = PlayerState.Alive;
        }

        public void SetName(string name)
        {
            nameTextMesh.text = name;
        }

        public void SetVelocity(Vector2 velocity)
        {
            if (Mathf.Abs(velocity.x) > 0.01f)
            {
                Vector3 localScale = graphicsTransform.localScale;
                localScale.x = initialXScale * Mathf.Sign(velocity.x);
                graphicsTransform.localScale = localScale;
            }

            graphicsAnimator.SetFloat("sqrSpeed", velocity.sqrMagnitude);
        }

        public void Die()
        {
            if (State == PlayerState.Alive)
            {
                State = PlayerState.Dead;
                Instantiate(disappearAnimationPrefab, graphicsTransform.position, Quaternion.identity);
                Destroy(gameObject); 
            }
        }
    }
}