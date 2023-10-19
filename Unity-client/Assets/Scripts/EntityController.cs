using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace BoomOnline2
{
    public abstract class EntityController : MonoBehaviour
    {
        public bool Valid { get; set; }

        public abstract void DestroySelf();
    }
}
