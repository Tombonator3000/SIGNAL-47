using UnityEngine;
using Signal47.Core;
namespace Signal47.Interaction
{
    public sealed class CameraPickup : MonoBehaviour,IInteractable
    {
        public string Prompt=>"COLLECT FIELD CAMERA";
        public void Interact(){GameSession.Instance.fieldCamera.TakeCamera();gameObject.SetActive(false);}
    }
}
