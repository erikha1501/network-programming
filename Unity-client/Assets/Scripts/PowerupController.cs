using BoomOnline2;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace BoomOnline2
{
    public class PowerupController : EntityController
    {
        public override void DestroySelf()
        {
            Destroy(this.gameObject);
        }
    }
}
