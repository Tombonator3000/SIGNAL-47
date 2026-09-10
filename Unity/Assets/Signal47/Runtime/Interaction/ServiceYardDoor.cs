using System.Collections;
using UnityEngine;
using Signal47.Core;
using Signal47.Investigation;

namespace Signal47.Interaction
{
    public sealed class ServiceYardDoor : MonoBehaviour, IInteractable
    {
        public Transform leaf;
        public ServiceYardInvestigation investigation;
        public bool Open { get; private set; }
        bool moving;
        public string Prompt => Open ? "SERVICE DOOR / OPEN" : investigation.Active ? "OPEN SERVICE DOOR" : "SERVICE DOOR / TRACKING LOCK";
        public void Interact()
        {
            if(!investigation.Active){GameSession.Instance.hud.Toast("Service access is locked during tracking.");return;}
            if(Open || moving)return;
            if(GameSession.Instance.director.palette)GameSession.Instance.director.palette.Click(true);
            StartCoroutine(Swing());
        }
        IEnumerator Swing()
        {
            moving=true;
            for(float t=0;t<.75f;t+=Time.deltaTime)
            {
                leaf.localRotation=Quaternion.Euler(0,-90*Mathf.SmoothStep(0,1,t/.75f),0);
                yield return null;
            }
            leaf.localRotation=Quaternion.Euler(0,-90,0);Open=true;moving=false;
        }
    }
}
