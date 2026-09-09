using UnityEngine;
using UnityEngine.InputSystem;
using Signal47.Core;
namespace Signal47.Interaction
{
    public sealed class PlayerInteractor : MonoBehaviour
    {
        public Camera viewCamera; public float distance=2.65f; IInteractable current;
        void Update()
        {
            if(GameSession.Instance==null || !GameSession.Instance.CanControl){current=null;if(GameSession.Instance?.hud!=null)GameSession.Instance.hud.InteractionPrompt="";return;}
            var mug=GameSession.Instance.director.mug;
            if(mug&&mug.Held){GameSession.Instance.hud.InteractionPrompt="E — RETURN MUG TO DESK";if(Keyboard.current!=null&&Keyboard.current.eKey.wasPressedThisFrame)mug.ReturnToDesk();return;}
            current=null;
            if(Physics.Raycast(viewCamera.transform.position,viewCamera.transform.forward,out var hit,distance,Physics.DefaultRaycastLayers,QueryTriggerInteraction.Ignore))
            {
                foreach(var b in hit.collider.GetComponentsInParent<MonoBehaviour>()) if(b is IInteractable i){current=i;break;}
            }
            GameSession.Instance.hud.InteractionPrompt=current==null?"":$"E — {current.Prompt}";
            if(current!=null && Keyboard.current!=null && Keyboard.current.eKey.wasPressedThisFrame) current.Interact();
        }
    }
}
