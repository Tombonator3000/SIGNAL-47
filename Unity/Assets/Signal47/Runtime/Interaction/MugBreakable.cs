using System.Collections;
using UnityEngine;
using Signal47.Core;
namespace Signal47.Interaction
{
    public sealed class MugBreakable:MonoBehaviour,IInteractable
    {
        public GameObject intact,broken;
        public bool Held{get;private set;}
        public bool IsBroken{get;private set;}
        public bool Falling{get;private set;}
        public string Prompt=>IsBroken?"EXAMINE BROKEN MUG":Held?"RETURN MUG TO DESK":"PICK UP COFFEE MUG";
        Vector3 rest;Quaternion restRotation;Transform originalParent;
        void Awake(){rest=transform.position;restRotation=transform.rotation;originalParent=transform.parent;}
        public void Interact()
        {
            if(Falling)return;
            if(IsBroken){GameSession.Instance.notebook.Add("The ceramic fragments match the break heard on the phone.");GameSession.Instance.hud.Toast("The coffee is still warm.");return;}
            if(Held){ReturnToDesk();return;}
            Held=true;transform.SetParent(GameSession.Instance.player.viewCamera.transform,false);transform.localPosition=new Vector3(.40f,-.49f,1.0f);transform.localRotation=Quaternion.Euler(-8,-16,0);
            GetComponent<Collider>().enabled=false;
        }
        public void ReturnToDesk()
        {
            if(Falling||IsBroken)return;
            Held=false;transform.SetParent(originalParent,true);transform.SetPositionAndRotation(rest,restRotation);GetComponent<Collider>().enabled=true;
        }
        public IEnumerator DropAndBreak()
        {
            if(Falling||IsBroken)yield break;
            bool wasHeld=Held;Falling=true;Held=false;transform.SetParent(originalParent,true);GetComponent<Collider>().enabled=false;
            Vector3 start=transform.position;Quaternion rot=transform.rotation;float t=0;
            Vector3 end=new Vector3(start.x,.025f,start.z+(wasHeld?.30f:-.85f));
            while(t<.7f){t+=Time.deltaTime;float n=Mathf.Clamp01(t/.7f);float fall=wasHeld?n:Mathf.Clamp01((n-.25f)/.75f);transform.position=new Vector3(start.x,Mathf.Lerp(start.y,end.y,fall*fall),Mathf.Lerp(start.z,end.z,Mathf.Clamp01(n*3)));transform.rotation=rot*Quaternion.Euler(n*125,n*45,n*160);yield return null;}
            transform.SetPositionAndRotation(end,Quaternion.identity);intact.SetActive(false);broken.SetActive(true);IsBroken=true;Falling=false;
            var box=GetComponent<BoxCollider>();box.size=new Vector3(.7f,.1f,.7f);box.center=new Vector3(0,.05f,0);box.enabled=true;
        }
    }
}
