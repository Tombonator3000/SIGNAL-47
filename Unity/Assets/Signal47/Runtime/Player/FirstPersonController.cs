using UnityEngine;
using UnityEngine.InputSystem;
using Signal47.Core;
namespace Signal47.Player
{
    [RequireComponent(typeof(CharacterController))]
    public sealed class FirstPersonController : MonoBehaviour
    {
        public Camera viewCamera; public float moveSpeed=3.15f; public float lookSensitivity=.085f;
        CharacterController cc; float pitch; float verticalVelocity;
        public float ViewPitch=>pitch;
        void Awake(){cc=GetComponent<CharacterController>();}
        public void RestorePose(Vector3 position,float yaw,float viewPitch)
        {
            bool wasEnabled=cc.enabled;cc.enabled=false;
            transform.SetPositionAndRotation(position,Quaternion.Euler(0,yaw,0));
            pitch=Mathf.Clamp(viewPitch,-70,70);verticalVelocity=0;
            viewCamera.transform.localRotation=Quaternion.Euler(pitch,0,0);
            cc.enabled=wasEnabled;Physics.SyncTransforms();
        }
        void Update()
        {
            if(GameSession.Instance==null || !GameSession.Instance.CanControl) return;
            if(Mouse.current!=null && Cursor.lockState==CursorLockMode.Locked)
            {
                Vector2 d=Mouse.current.delta.ReadValue()*lookSensitivity;
                transform.Rotate(0,d.x,0); pitch=Mathf.Clamp(pitch-d.y,-70,70); viewCamera.transform.localRotation=Quaternion.Euler(pitch,0,0);
            }
            var k=Keyboard.current;if(k==null)return;
            Vector2 input=new((k.dKey.isPressed?1:0)-(k.aKey.isPressed?1:0),(k.wKey.isPressed?1:0)-(k.sKey.isPressed?1:0));
            if(input.sqrMagnitude>1)input.Normalize();Vector3 move=transform.right*input.x+transform.forward*input.y;
            if(cc.isGrounded)verticalVelocity=-1f;else verticalVelocity+=Physics.gravity.y*Time.deltaTime;
            move*=moveSpeed;move.y=verticalVelocity;cc.Move(move*Time.deltaTime);
        }
    }
}
