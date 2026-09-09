using UnityEngine;
using Signal47.Core;
namespace Signal47.Interaction
{
    public sealed class PhoneInteractable:MonoBehaviour,IInteractable
    {
        public Transform handset;
        Vector3 rest;Quaternion restRotation;
        public bool OffHook{get;private set;}
        public string Prompt=>GameSession.Instance.director.PhoneRinging?"ANSWER DESK PHONE":"DESK PHONE";
        void Awake(){if(handset){rest=handset.localPosition;restRotation=handset.localRotation;}}
        void Update()
        {
            if(!handset||!GameSession.Instance)return;
            var d=GameSession.Instance.director;
            OffHook=d.PhoneAnswered&&!d.LineDead;
            float shake=d.PhoneRinging?Mathf.Sin(Time.time*37)*.008f:0;
            var target=rest+(OffHook?new Vector3(0,.32f,.08f):new Vector3(shake,0,0));
            handset.localPosition=Vector3.Lerp(handset.localPosition,target,1-Mathf.Exp(-Time.deltaTime*7));
            handset.localRotation=Quaternion.Slerp(handset.localRotation,restRotation*Quaternion.Euler(OffHook?new Vector3(-18,0,12):Vector3.zero),1-Mathf.Exp(-Time.deltaTime*7));
        }
        public void Interact(){var d=GameSession.Instance.director;if(d.PhoneRinging)d.AnswerPhone();else GameSession.Instance.hud.Toast(d.PhoneAnswered?"No dial tone.":"Internal line. No call.");}
    }
}
