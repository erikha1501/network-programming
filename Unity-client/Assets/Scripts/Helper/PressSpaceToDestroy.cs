using System;
using UnityEngine;
using UnityEngine.Events;

public class PressSpaceToDestroy : MonoBehaviour
{
    [SerializeField]
    private UnityEvent destroyHandler;

    private void Update()
    {
        if (Input.GetKeyDown(KeyCode.Space))
        {
            destroyHandler?.Invoke();
        }
    }
}
