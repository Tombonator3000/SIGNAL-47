using UnityEngine;
using Signal47.Core;
namespace Signal47.Interaction
{
    public sealed class PhoneInteractable:MonoBehaviour,IInteractable
    {
        public Transform handset;
        public LineRenderer receiverLead;
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
            // The authored phone is now life-sized. Keep the receiver lift in metres
            // independently of the model's scale, in the phone's facing direction.
            var offset=transform.TransformDirection(OffHook?new Vector3(0,.32f,.08f):new Vector3(shake,0,0));
            var target=rest+handset.parent.InverseTransformVector(offset);
            handset.localPosition=Vector3.Lerp(handset.localPosition,target,1-Mathf.Exp(-Time.deltaTime*7));
            if(receiverLead)
            {
                var socket=new Vector3(.33f,.325f,-.15f);
                var a=transform.TransformPoint(socket);var b=handset.TransformPoint(socket);
                receiverLead.enabled=Vector3.Distance(a,b)>.006f;
                for(int i=0;i<receiverLead.positionCount;i++)
                {
                    float t=(float)i/(receiverLead.positionCount-1);
                    receiverLead.SetPosition(i,Vector3.Lerp(a,b,t)-Vector3.up*Mathf.Sin(t*Mathf.PI)*.035f);
                }
            }
            handset.localRotation=Quaternion.Slerp(handset.localRotation,restRotation*Quaternion.Euler(OffHook?new Vector3(-18,0,12):Vector3.zero),1-Mathf.Exp(-Time.deltaTime*7));
        }
        public void Interact(){var d=GameSession.Instance.director;if(d.PhoneRinging)d.AnswerPhone();else GameSession.Instance.hud.Toast(d.PhoneAnswered?"No dial tone.":"Internal line. No call.");}
    }
}
